/**
 * @file sul_base.h
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief Base class for the system under learning.
 * @version 0.1
 * @date 2023-02-15
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _SUL_BASE_H_
#define _SUL_BASE_H_

#include "misc/sqldb.h"
#include "source/input/inputdata.h"

#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

class sul_base {
    friend class base_teacher;
    friend class teacher_imat;
    friend class eq_oracle_base;

  protected:
    virtual void reset() = 0;

    std::ifstream get_input_stream() const;

    virtual bool is_member(const std::vector<int>& query_trace) const = 0;

    virtual const int query_trace(const std::vector<int>& query_trace, inputdata& id) const;

    /** Query_trace, but in case the query is not available returns -1 instead of throwing an error */
    virtual const int query_trace_maybe(const std::vector<int>& query_trace, inputdata& id) const;

    virtual const std::pair<int, std::vector<float>> get_type_and_state(const std::vector<int>& query_trace,
                                                                        inputdata& id) const;

    // TODO: can we simplify all of those with some sort of template?
    virtual const double get_string_probability(const std::vector<int>& query_trace, inputdata& id) const;
    virtual const std::vector<float> get_weight_distribution(const std::vector<int>& query_trace, inputdata& id) const;
    virtual const std::pair<std::vector<float>, std::vector<float>>
    get_weights_and_state(const std::vector<int>& query_trace, inputdata& id) const;

  public:
    sul_base() = default; // abstract anyway

    /**
     * @brief Initialize the sul class.
     */
    virtual void pre(inputdata& id) = 0;
};

#endif
