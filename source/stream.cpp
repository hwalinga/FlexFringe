// TODO: those are a lot of includes. Do we need them all?
#include "stream.h"
#include <cstdlib>

#include "greedy.h"
#include "state_merger.h"
#include "evaluate.h"
#include "dfasat.h"
#include "evaluation_factory.h"
#include "searcher.h"
#include "parameters.h"

#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stack>

#include <chrono> // for measuring performance
#include <fstream> // for measuring performance

const bool RETRY_MERGES = true; // if true use new streaming scheme, else use the old one
const bool EXPERIMENTAL_RUN = true;
void stream_object::greedyrun_no_undo(state_merger* merger, const int seq_nr, const bool last_sequence, const int n_runs){
    static int count_printouts = 1;

    int c2 = 0;

    refinement* top_ref;
    top_ref = merger->get_best_refinement();
    while(top_ref != 0){
      top_ref->doref(merger);
      top_ref = merger->get_best_refinement();
    }

    if(seq_nr % 100 == 0  || last_sequence){
      cout << "Processed " << count_printouts << " batches" << endl;
      ++count_printouts;
    }
}

void print_current_automaton_stream(state_merger* merger, const string& output_file, const string& append_string){
    if (OUTPUT_TYPE == "dot" || OUTPUT_TYPE == "both") {
        merger->print_dot(output_file + append_string + ".dot");
    }
    if (OUTPUT_TYPE == "json" || OUTPUT_TYPE == "both") {
        merger->print_json(output_file + append_string + ".json");
    }
    if(OUTPUT_SINKS && !PRINT_WHITE){
        bool red_undo = PRINT_RED;
        PRINT_RED = false;
        bool white_undo = PRINT_WHITE;
        PRINT_WHITE= true;
        bool blue_undo = PRINT_BLUE;
        PRINT_BLUE = true;
        if (OUTPUT_TYPE == "dot" || OUTPUT_TYPE == "both") {
            merger->print_dot(output_file + append_string + "sinks.dot");
        }
        if (OUTPUT_TYPE == "json" || OUTPUT_TYPE == "both") {
            merger->print_json(output_file + append_string + "sinks.json");
        }
        PRINT_RED = red_undo;
        PRINT_WHITE = white_undo;
        PRINT_BLUE = blue_undo;
    }
}

void stream_object::greedyrun_retry_merges(state_merger* merger, const int seq_nr, const bool last_sequence, const int n_runs){
    static int count_printouts = 1;

    static int c = 0;
    int c2 = 0;

    stack< refinement* > refinement_stack;
    queue<refinement*> failed_refs;

    static ofstream tree_size_doc("tree_size_per_batch.txt");
    refinement* top_ref;

    this->states_to_append_to.clear();

    // for tracking the refinements
    static ofstream outf;
    string recomputed_merge = "nr"; // nr for no_recompute, r for recompute
    bool track_refinements = (n_runs == 47654 || n_runs == 47655 || n_runs == 46066 || n_runs == 46067 || n_runs == 51696 || n_runs == 51695);
    if(track_refinements){
      std::ostringstream oss; // for tracking refinements (debugging)
      oss << "refinements_at_" << n_runs << "_full.txt";
      outf.open(oss.str().c_str());
    }

    if(currentrun->empty()){
      top_ref = merger->get_best_refinement();
    }
    else{
        top_ref = currentrun->front();
        currentrun->pop_front();
    }

    while(top_ref != 0){
        if(top_ref->test_ref_structural(merger)){
          if(top_ref->test_ref_consistency(merger)){
            nextrun->push_back(top_ref);
            refinement_stack.push(top_ref);
            top_ref->doref(merger);
          }
        }
        else{
          failed_refs.push(top_ref);
        }

        if(currentrun->empty()){
            top_ref = 0;
        }
        else{
            top_ref = currentrun->front();
            currentrun->pop_front();
        }
    }

    top_ref = merger->get_best_refinement();
    while(top_ref != 0){
      nextrun->push_back(top_ref);
      refinement_stack.push(top_ref);
      top_ref->doref(merger);

      queue<refinement*> tmp;
      while(!failed_refs.empty()){
        top_ref = failed_refs.front();
        failed_refs.pop();
        if(top_ref->test_ref_structural(merger)){
          if(top_ref->test_ref_consistency(merger)){
            nextrun->push_back(top_ref);
            refinement_stack.push(top_ref);
            top_ref->doref(merger);
          }
        }
        else{
          tmp.push(top_ref);
        }
      }
      failed_refs = tmp;

      top_ref = merger->get_best_refinement();
    }

    ++count_printouts;
    if(count_printouts % 100 == 0  || last_sequence){
      cout << "Processed " << count_printouts << " batches" << endl;
    }

    if(count_printouts % 1000 == 0){
      print_current_automaton_stream(merger, "model_batch_nr_", to_string(count_printouts));
    }

    // undo the merges
    while(!refinement_stack.empty()){
      top_ref = refinement_stack.top();
      top_ref->undo(merger);
      refinement_stack.pop();

      states_to_append_to.insert(top_ref->red->get_number());
      if(top_ref->type() == refinement_type::merge_rf_type){
        states_to_append_to.insert(dynamic_cast<merge_refinement*>(top_ref)->blue->get_number());
      }
    }

    if(track_refinements){
      outf.close();
    }

    delete currentrun;
    currentrun = nextrun;
    nextrun = new refinement_list();
}

/**
 * @brief Runs the stream mode.
 * 
 * This is the original version of the streaming function, where we can toggle the old and the new version.
 * 
 * @param merger The selected state merger instance.
 * @param param The global parameters.
 * @param input_stream The input data file.
 * @param id Input-data wrapper object. 
 * @return int 
 */
int stream_object::stream_mode(state_merger* merger, ifstream& input_stream, inputdata* id, parser* input_parser) {
    unsigned int seq_nr = 0;
    bool last_sequence = false;

    // for performance measurement
    unsigned int n_runs = 0;

    while(true) {
      int read_lines = 0;
      while (read_lines < BATCH_SIZE){
        if(input_stream.eof()){
          last_sequence = true;
          break; // TODO: for experiments, delete afterwards
        }

        ++read_lines;
        ++seq_nr;
        
        std::optional<trace*> trace_opt = id->read_trace(*input_parser, *parser_strategy);

        if(EXPERIMENTAL_RUN && !trace_opt){
          last_sequence = true;
          break;
        }
        else{
          while(!trace_opt) trace_opt = id->read_trace(*input_parser, *parser_strategy); // making sure we got a value; for real streaming
        }
        trace* new_trace = trace_opt.value();
        new_trace->sequence = seq_nr;
        
        id->add_trace_to_apta(new_trace, merger->get_aut(), this->states_to_append_to);
        if(!ADD_TAILS) new_trace->erase();
      }

      if(RETRY_MERGES) greedyrun_retry_merges(merger, seq_nr, last_sequence, n_runs);
      else greedyrun_no_undo(merger, seq_nr, last_sequence, n_runs);
      ++(this->batch_number);
      ++n_runs;

      if(input_stream.eof()){
        if(RETRY_MERGES){
          // one more step, because we undid refinements earlier
          greedyrun_retry_merges(merger, seq_nr, last_sequence, n_runs);

          if(currentrun->size() == 0){
            cerr << "Error: Currentrun object empty after running the whole input. No output to be generated." << endl;
            throw new exception;
          }

          for(auto top_ref: *currentrun){
              top_ref->doref(merger);
          }
        }

        cout << "Finished parsing file. End of program." << endl;
        return 0;
      }
    }
    return 0;
}
