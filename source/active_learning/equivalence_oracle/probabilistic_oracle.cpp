/**
 * @file probabilistic_oracle.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-04-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "probabilistic_oracle.h"
#include "common_functions.h"

#include <cmath>

using namespace std;
using namespace active_learning_namespace;

/**
 * @brief This function does what you think it does. The type does not matter, because we perform this one 
 * on SULs that return us probabilities, hence it only returns a dummy value.
 * 
 * TODO: this function is also inefficient in terms of inferring probabilities. Optimize this one.
 * 
 * @param merger The merger.
 * @param teacher The teacher.
 * @return std::optional< pair< vector<int>, int> > nullopt if no counterexample found, else the counterexample. The type int will 
 * always be zero.
 */
std::optional< pair< vector<int>, int> > probabilistic_oracle::equivalence_query(state_merger* merger, const unique_ptr<base_teacher>& teacher) {  
  inputdata& id = *(merger->get_dat());
  apta& hypothesis = *(merger->get_aut());

  std::optional< vector<int> > query_string_opt = search_strategy->next(id);
  while(query_string_opt != nullopt){ // nullopt == search exhausted
    auto& query_string = query_string_opt.value();
    double true_val = teacher->get_string_probability(query_string, id);
    
    if(true_val < 0) return make_optional< pair< vector<int>, int> >(query_string, true_val); // target automaton cannot be parsed with this query string

    trace* test_tr = vector_to_trace(query_string, id, 0); // type-argument irrelevant here

    apta_node* n = hypothesis.get_root();
    tail* t = test_tr->get_head();

    vector<int> current_substring;
    for(int j = 0; j < test_tr->get_length(); j++){
        n = active_learning_namespace::get_child_node(n, t);
        
        if(n == nullptr){
          cout << "Counterexample because tree not parsable" << endl;
          search_strategy->reset();
          return make_optional< pair< vector<int>, int > >(make_pair(query_string, 0));
        } 

        current_substring.push_back(t->get_symbol());
        const double true_p = teacher->get_string_probability(substring, id);

        trace* substring_tr = vector_to_trace(current_substring);
        const double inferred_p = get_probability(substring_tr, id, teacher);
        mem_store::delete_trace(substring_tr);

        if( abs(true_p - inferred_p) > max_distance){
          cout << "Predictions of the following counterexample: The true probability: " << true_p << ", predicted probability: " << inferred_p << endl;
          search_strategy->reset();
          return make_optional< pair< vector<int>, int > >(make_pair(query_string, 0));
        } 

        t = t->future();
    }

    query_string_opt = search_strategy->next(id);
  }

  return nullopt;
}
