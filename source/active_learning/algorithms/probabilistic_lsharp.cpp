/**
 * @file lsharp.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief The (strategic) L#-algorithm, as described by Vandraager et al. (2022): "A New Approach for Active Automata Learning Based on Apartness"
 * @version 0.1
 * @date 2023-03-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "lsharp.h"
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

#include <list>
#include <stdexcept>

using namespace std;
using namespace active_learning_namespace;


/**
 * @brief Take a node and complete it wrt to the alphabet.
 * 
 * TODO: shall we check if we already have this node? If yes, then we don't need to do anything.
 */
void probabilistic_lsharp_algorithm::complete_state(unique_ptr<state_merger>& merger, apta_node* n, inputdata& id, const vector<int>& alphabet) const {
  if(completed_nodes.contains(n)) return;

  for(const int symbol: alphabet){
    if(n->get_child(symbol) == nullptr){
      auto access_trace = n->get_access_trace();

      pref_suf_t seq;
      if(n->get_number() != -1) seq = access_trace->get_input_sequence(true, true);
      else seq.resize(1);
      
      seq[seq.size()-1] = symbol;
      
      trace* new_trace = vector_to_trace(seq, id);
      id.add_trace(new_trace);
      
      const double new_prob = get_probability(new_trace, id, teacher, merger->get_aut());
      for(int i=0; i<int(answer*new_prob); ++i) 
        id.add_trace_to_apta(new_trace, merger->get_aut(), false, true); // for-loop needed for probabilistic version
    }
  }
  completed_nodes.insert(n);
}

void probabilistic_lsharp_algorithm::update_state(std::unique_ptr<state_merger>& merger, apta_node* n, inputdata& id, const std::vector<int>& alphabet) const {
  // TODO: I will implement this later. A tool to counter counterexamples
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
void probabilistic_lsharp_algorithm::proc_counterex(const unique_ptr<base_teacher>& teacher, inputdata& id, unique_ptr<apta>& hypothesis, 
                                      const vector<int>& counterex, unique_ptr<state_merger>& merger, const refinement_list refs, 
                                      const vector<int>& alphabet) const {
  // in this block we do a linear search for the fringe of the prefix tree. Once we found it, we ask membership queries for each substring
  // of the counterexample (for each new state that we create), and this way add the whole counterexample to the prefix tree
  reset_apta(merger.get(), refs);
  vector<int> substring;
  apta_node* n = hypothesis->get_root();
  for(auto s: counterex){
    if(n == nullptr){
      trace* new_trace = vector_to_trace(substring, id);
      id.add_trace(new_trace);

      const double new_prob = get_probability(new_trace, id, teacher, hypothesis.get()); // TODO: we want to complete this guy instead
      for(int i=0; i<int(new_prob*sample_size; ++i)) id.add_trace_to_apta(new_trace, hypothesis.get(), false, true);
      substring.push_back(s);
    }
    else{
      // find the fringe
      substring.push_back(s);
      trace* parse_trace = vector_to_trace(substring, id, 0); // TODO: inefficient like this
      tail* t = substring.size() == 0 ? parse_trace->get_end() : parse_trace->get_end()->past_tail;

      update_state(merger, n, id, alphabet);

      n = active_learning_namespace::get_child_node(n, t);
      mem_store::delete_trace(parse_trace);
    } 
  }

  // for the last element, too
  const auto queried_type = teacher->get_string_probability(substring, id); // TODO: we want to complete this guy instead
  trace* new_trace = vector_to_trace(substring, id, queried_type);
  id.add_trace(new_trace);
  for(int i=0; i<int(answer*sample_size; ++i)) id.add_trace_to_apta(new_trace, hypothesis.get(), false, true);

  // TODO: alternatively implement the binary search here
}