/**
 * @file input_file_oracle.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-03-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "input_file_oracle.h"
#include "predict.h"
#include "common_functions.h"

using namespace std;
using namespace active_learning_namespace;

bool input_file_oracle::apta_accepts_trace(state_merger* merger, const vector<int>& tr, inputdata& id) const {
  const static double THRESH = 1; // TODO: we need to set this guy somehow

  trace* new_tr = vector_to_trace(tr, id, 1);
  double score = predict_trace(merger, new_tr);
  mem_store::delete_trace(new_tr);

  return score < THRESH;
}

optional< pair< vector<int>, int > > input_file_oracle::equivalence_query(state_merger* merger) {
  const count_driven* const eval_func = dynamic_cast<count_driven*>(merger->get_eval());
  if(eval_func == nullptr) throw logic_error("Must have a heuristic that derives from count_driven at this point.");

  for(const auto& [sequence, type]: dynamic_cast<input_file_sul*>(sul)->get_all_traces()){
    trace* tr = vector_to_trace(sequence, *(merger->get_dat()), type);
    if(!active_learning_namespace::aut_accepts_trace(tr, merger->get_aut(), eval_func)){
      return make_optional< pair< vector<int>, int > >(make_pair(sequence, type));
    }
  }
  return nullopt; // empty optional
}