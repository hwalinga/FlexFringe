/**
 * @file w_method.h
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief 
 * @version 0.1
 * @date 2023-04-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _AL_W_METHOD_SEARCH_H_
#define _AL_W_METHOD_SEARCH_H_

#include "search_base.h"
#include "random_int_generator.h"

#include <random>
#include <vector>

class w_method : public search_base {
  private:
    int samples_drawn;
    int max_samples;
    int last_lower_bound; // for optimization purposes

    random_int_generator length_generator;
    random_int_generator alphabet_sampler;

    std::vector<int> alphabet_vec;

  public:
    w_method(const int max_depth) : search_base(max_depth) {
      samples_drawn = 0;
      last_lower_bound = 10;

      length_generator.set_limits(last_lower_bound, MAX_SEARCH_DEPTH);

      max_samples = 500000;
    };

    virtual std::optional< std::list<int> > next(const inputdata& id) override;
    std::optional< std::list<int> > next(const inputdata& id, const int lower_bound);
};

#endif
