/**
 * @file common_functions.h
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-02-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _COMMON_FUNCTIONS_H_
#define _COMMON_FUNCTIONS_H_

#include "evaluate.h"
#include "parameters.h"
#include "apta.h"
#include "tail.h"
#include "trace.h"
#include "refinement.h"
#include "definitions.h"
#include "count_types.h"
#include "inputdata.h"

#include <vector>
#include <list>
#include <map>

namespace active_learning_namespace{

  apta_node* get_child_node(apta_node* n, tail* t);
  bool aut_accepts_trace(trace* tr, apta* aut); 
  bool aut_accepts_trace(trace* tr, apta* aut, const count_driven* const eval); 

  const int predict_type_from_trace(trace* tr, apta* aut, inputdata& id); 

  void reset_apta(state_merger* merger, const std::list<refinement*>& refs);
  const std::list<refinement*> minimize_apta(state_merger* merger);

  std::vector<int> concatenate_strings(const std::vector<int>& pref1, const std::vector<int>& pref2);
  trace* vector_to_trace(const std::vector<int>& vec, inputdata& id, const int trace_type);

  void add_sequence_to_trace(trace* new_trace, const std::vector<int> sequence);
  void update_tail(/*out*/ tail* t, const int symbol);

  void print_vector(const vector<int>& v);
}

#endif