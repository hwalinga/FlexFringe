/**
 * @file base_teacher.cpp
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-02-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "base_teacher.h"

using namespace std;
using namespace active_learning_namespace;

/**
 * @brief Asks the teacher a membership query. In case the automaton cannot be parsed with the query, it returns -1. Else the mapping as 
 * defined by the inputdata.
 * 
 * @param prefix S.e.
 * @param suffix S.e.
 * @param id Inputdata.
 * @return const int 0 or greater for the type, -1 if automaton cannot be parsed with the query.
 */
const int base_teacher::ask_membership_query(const pref_suf_t& prefix, const pref_suf_t& suffix, inputdata& id) {
  pref_suf_t query(prefix);
  query.insert(query.end(), suffix.begin(), suffix.end());

  return ask_membership_query(query, id);
}

/**
 * @brief See ask_membership_query(const pref_suf_t& prefix, const pref_suf_t& suffix, inputdata& id) 
 * 
 * @param query 
 * @param id 
 * @return const int 0 or greater for the type, -1 if automaton cannot be parsed with the query.
 */
const int base_teacher::ask_membership_query(const pref_suf_t& query, inputdata& id) {
  return sul->query_trace(query, id);
}

/**
 * @brief Gets the probability of a certain string. Only supported with certain SULs.
 * 
 * @param query 
 * @param id 
 * @return const double 
 */
const double base_teacher::get_string_probability(const pref_suf_t& query, inputdata& id) {
  double res = sul->get_string_probability(query, id);
  int tmp = res * 100000.0; // 44.8 truncated to 44
  double truncated = tmp / 100000.0; // 4.4
  return truncated; //log(truncated);
}