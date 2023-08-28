#ifndef _STREAM_
#define _STREAM_

#include "refinement.h"
#include "state_merger.h"
#include "parameters.h"
#include "input/parsers/reader_strategy.h"
#include "active_learning/system_under_learning/database_sul.h"

#include <sstream>
#include <stack>
#include <unordered_set>
#include <memory>

/**
 * @brief Class to realize the streaming mode, when observing a stream of data, e.g. network data.
 * 
 */
class stream_object{

private:
  int batch_number; // TODO: naming
  refinement_list* currentrun;
  refinement_list* nextrun;

  std::unordered_set<int> states_to_append_to; // keeping track of states that we can append to with ease
  reader_strategy* parser_strategy;

  void greedyrun_no_undo(state_merger* merger, const int seq_nr, const bool last_sequence, const int n_runs);
  void greedyrun_retry_merges(state_merger* merger, const int seq_nr, const bool last_sequence, const int n_runs); // for experiments

  refinement* determine_next_refinement(state_merger* merger);

  void remember_state(refinement* ref);

public:
  /**
   * @brief Construct a new stream object object
   * 
   */
  stream_object(){
    batch_number = 0;

    // TODO: make those smart pointers
    currentrun = new refinement_list();
    nextrun = new refinement_list();

    parser_strategy = new in_order();
  }

  ~stream_object(){
    delete currentrun;
    delete nextrun;
    delete parser_strategy;
  }
  
  int stream_mode(state_merger* merger, ifstream& input_stream, inputdata* id, parser* input_parser); 
};

#endif