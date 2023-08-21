/**
 * @file breadth_first_search.h
 * @author Robert Baumgartner (r.baumgartner-1@tudelft.nl)
 * @brief Inefficient.
 * @version 0.1
 * @date 2023-04-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _AL_BREADTH_FIRST_SEARCH_H_
#define _AL_BREADTH_FIRST_SEARCH_H_

#include "search_base.h"

#include <list>
#include <stack>

class bfs_strategy : public search_base {
  private:
    int depth;

    std::stack< std::list<int> > curr_search;
    std::stack< std::list<int> > old_search;
  public:
    bfs_strategy(const int max_depth) : search_base(max_depth) {
      depth = 0;
    };

    virtual std::optional< std::list<int> > next(const inputdata& id) override;
};

#endif
