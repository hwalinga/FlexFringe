/**
 * @file weighted_lsharp.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-11-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "weighted_lsharp.h"
#include "base_teacher.h"
#include "input_file_sul.h"
#include "common_functions.h"
#include "input_file_oracle.h"

#include "state_merger.h"
#include "parameters.h"
#include "inputdata.h"
#include "mem_store.h"
#include "greedy.h"
#include "main_helpers.h"

#include "weight_comparator.h"

#include <list>
#include <stdexcept>

using namespace std;
using namespace active_learning_namespace;


/**
 * @brief Take a node and complete it wrt to the alphabet.
 * 
 * This is the only function that creates new nodes (apart from the root node).
 * 
 */
unordered_set<apta_node*> weighted_lsharp_algorithm::extend_fringe(unique_ptr<state_merger>& merger, apta_node* n, unique_ptr<apta>& the_apta, inputdata& id, const vector<int>& alphabet) const {
  static unordered_set<apta_node*> extended_nodes; // TODO: we might not need this, since it is inherently backed in query_weights and the size of traces
  if(extended_nodes.contains(n)) return unordered_set<apta_node*>();
  
  unordered_set<apta_node*> new_nodes;

  // this path is only relevant in traces that were added via counterexample processing
  auto access_trace = n->get_access_trace();

  pref_suf_t seq;
  if(n->get_number() != -1 && n->get_number() != 0) seq = access_trace->get_input_sequence(true, true);
  else seq.resize(1);

  for(const int symbol: alphabet){
    seq[seq.size()-1] = symbol;
    trace* new_trace = vector_to_trace(seq, id);
    id.add_trace_to_apta(new_trace, merger->get_aut(), false);

    query_weights(merger, n->get_child(symbol), id, alphabet, seq);
    new_nodes.insert(n->get_child(symbol));
  }
  return new_nodes;
}


/**
 * @brief Adds statistics to a node, returns the traces that were queried along the way.
 * 
 * @param n The node to upate.
 * @return optional< vector<trace*> > A vector with all the new traces we added to the apta, or nullopt if we already processed this node. Saves runtime.
 */
void weighted_lsharp_algorithm::query_weights(unique_ptr<state_merger>& merger, apta_node* n, inputdata& id, const vector<int>& alphabet, optional<pref_suf_t> seq_opt) const {  
  static unordered_set<apta_node*> completed_nodes;
  if(completed_nodes.contains(n)) return;
  static const int SOS = START_SYMBOL == -1 ? -1 : START_SYMBOL;
  static const int EOS = END_SYMBOL == -1 ? -1 : END_SYMBOL;

  auto* data = static_cast<weight_comparator_data*>(n->get_data());
  data->initialize_weights(alphabet);

  pref_suf_t seq;
  if(seq_opt){
    seq = pref_suf_t(seq_opt.value());
  }
  else{
    auto access_trace = n->get_access_trace();
    seq = access_trace->get_input_sequence(true, false);
  }

  const vector<float> weights = teacher->get_weigth_distribution(seq, id);    
  for(int i = 0; i < weights.size(); ++i){
    if(SOS > -1 && i==SOS) continue;
    else if(i==EOS){
      data->set_final_weight(weights[i]);
      continue;
    }
    int symbol = id.symbol_from_string(to_string(i));
    //cout << "Symbol: " << symbol << ", weights-size: " << weights.size() << endl;
    data->set_weight(symbol, weights[i]);
  }
  completed_nodes.insert(n);
}


/**
 * @brief Processing the counterexample recursively in the binary search strategy 
 * as described by the paper.
 * 
 * Operations done directly on the APTA.
 * 
 * @param aut The merged APTA.
 * @param counterex The counterexample.
 */
void weighted_lsharp_algorithm::proc_counterex(const unique_ptr<base_teacher>& teacher, inputdata& id, unique_ptr<apta>& hypothesis, 
                                      const vector<int>& counterex, unique_ptr<state_merger>& merger, const refinement_list refs,
                                      const vector<int>& alphabet) const {
  // linear search to find fringe, then append new states
  cout << "proc counterex" << endl;
  reset_apta(merger.get(), refs);

  return;

  vector<int> substring;
  apta_node* n = hypothesis->get_root();
  for(auto s: counterex){
    if(n == nullptr){
      const auto queried_type = teacher->ask_membership_query(substring, id);
      trace* new_trace = vector_to_trace(substring, id, queried_type);
      id.add_trace_to_apta(new_trace, hypothesis.get(), false);
      id.add_trace(new_trace);
      substring.push_back(s);
    }
    else{
      // find the fringe
      substring.push_back(s);
      trace* parse_trace = vector_to_trace(substring, id, 0); // TODO: inefficient like this
      tail* t = substring.size() == 0 ? parse_trace->get_end() : parse_trace->get_end()->past_tail;
      n = active_learning_namespace::get_child_node(n, t);
      mem_store::delete_trace(parse_trace);
    }
  }

  // for the last element, too
  const auto queried_type = teacher->ask_membership_query(substring, id);
  trace* new_trace = vector_to_trace(substring, id, queried_type);
  id.add_trace_to_apta(new_trace, hypothesis.get(), false);
  id.add_trace(new_trace);

  // now let's walk over the apta again, completing all the states we created
  n = hypothesis->get_root();
  trace* parsing_trace = vector_to_trace(counterex, id);
  tail* t = parsing_trace->get_head();
  //double product = 1;

  while(n!=nullptr){
    query_weights(merger, n, id, alphabet, nullopt);
    n = active_learning_namespace::get_child_node(n, t);
    t = t->future();
  }
}

/**
 * @brief Does what you think it does.
 * 
 * @param merger 
 * @param the_apta 
 * @return list<refinement*> 
 */
list<refinement*> weighted_lsharp_algorithm::find_complete_base(unique_ptr<state_merger>& merger, unique_ptr<apta>& the_apta,
                                                                     inputdata& id, const vector<int>& alphabet) {
  static int depth = 1; // because we initialize root node with depth 1
  static const int MAX_DEPTH = 10;
  
  list<refinement*> performed_refs;
  unordered_set<apta_node*> fringe_nodes;

  while(true){ // cancel when either not red node identified or max depth

    unordered_set<apta_node*> blue_nodes; 
    for(blue_state_iterator b_it = blue_state_iterator(the_apta->get_root()); *b_it != nullptr; ++b_it){
      auto blue_node = *b_it;
      blue_nodes.insert(blue_node);
    }

    if(blue_nodes.empty()){
      return performed_refs;
    }
    else if(depth == MAX_DEPTH){
      cout << "Max-depth reached. Printing the automaton. Depth:" << depth << endl;
      find_closed_automaton(performed_refs, the_apta, merger, weight_comparator::get_distance);
      print_current_automaton(merger.get(), OUTPUT_FILE, ".final");
      cout << "Printed. Terminating" << endl;
      exit(1);
    }

    bool identified_red_node = false;
    for(auto blue_node: blue_nodes){

      refinement_set possible_merges;
      for(red_state_iterator r_it = red_state_iterator(the_apta->get_root()); *r_it != nullptr; ++r_it){
        const auto red_node = *r_it;
        
        refinement* ref = merger->test_merge(red_node, blue_node);
        if(ref != nullptr){
          possible_merges.insert(ref);
        }
      }

      if(possible_merges.size()==0){
        identified_red_node = true;
        refinement* ref = mem_store::create_extend_refinement(merger.get(), blue_node);
        ref->doref(merger.get());
        performed_refs.push_back(ref);

        extend_fringe(merger, blue_node, the_apta, id, alphabet);
      }
      else{
        // get the best refinement from the heap
        refinement* best_merge =  *(possible_merges.begin());
        for(auto it : possible_merges){
            if(it != best_merge) it->erase();
        }
        best_merge->doref(merger.get());
        performed_refs.push_back(best_merge);
      }
    }

    /* {
      static int model_nr = 0;
      print_current_automaton(merger.get(), "model.", to_string(++model_nr) + ".after_refs");
    } */

    if(!identified_red_node){
      cout << "Complete basis found. Forwarding hypothesis" << endl;
      return performed_refs;
    }
    else{
      cout << "Red node identified in iteration. Continue search in depth " << ++depth << endl;
    }
  }
}


/**
 * @brief Main routine of this algorithm.
 * 
 * @param id Inputdata.
 */
void weighted_lsharp_algorithm::run(inputdata& id){
  int n_runs = 1;
  
  auto eval = unique_ptr<evaluation_function>(get_evaluation());
  eval->initialize_before_adding_traces();
  
  auto the_apta = unique_ptr<apta>(new apta());
  auto merger = unique_ptr<state_merger>(new state_merger(&id, eval.get(), the_apta.get()));

  const vector<int> alphabet = id.get_alphabet();
  cout << "Alphabet: ";
  active_learning_namespace::print_sequence< vector<int>::const_iterator >(alphabet.cbegin(), alphabet.cend());

  {
    // init the root node
    auto root_node = the_apta->get_root();
    pref_suf_t seq;
    query_weights(merger, root_node, id, alphabet, seq);
    extend_fringe(merger, root_node, the_apta, id, alphabet);
  }

  while(ENSEMBLE_RUNS > 0 && n_runs <= ENSEMBLE_RUNS){
    if( n_runs % 100 == 0 ) cout << "Iteration " << n_runs + 1 << endl;

    auto refs = find_complete_base(merger, the_apta, id, alphabet);

    cout << "Searching for counterexamples" << endl;

    // only merges performed, hence we can test our hypothesis
    while(true){
      /* While loop to check the type. type is < 0 if sul cannot properly respond to query, e.g. when the string 
      we ask cannot be parsed in automaton. We ignore those cases, as they lead to extra states in hypothesis. 
      This puts a burden on the equivalence oracle to make sure no query is asked twice, else we end 
      up in infinite loop.*/
      optional< pair< vector<int>, int > > query_result = oracle->equivalence_query(merger.get(), teacher);
      if(!query_result){
        cout << "Found consistent automaton => Print." << endl;
        print_current_automaton(merger.get(), OUTPUT_FILE, ".final"); // printing the final model each time
        return;
      }

      const int type = query_result.value().second;
      if(type < 0) continue;

      const vector<int>& cex = query_result.value().first;
      cout << "Counterexample of length " << cex.size() << " found: ";
      print_vector(cex);
      proc_counterex(teacher, id, the_apta, cex, merger, refs, alphabet);

      break;
    }

    // TODO: this one might not hold
    ++n_runs;
    if(ENSEMBLE_RUNS > 0 && n_runs == ENSEMBLE_RUNS){
      cout << "Maximum of runs reached. Printing automaton." << endl;
      print_current_automaton(merger.get(), OUTPUT_FILE, ".final");
      return;
    }
  }
}
