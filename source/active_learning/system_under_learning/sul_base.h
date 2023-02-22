/**
 * @file run_active_learning.h
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

class teacher_base;

class sul_base{
  friend class teacher_base;

  protected:
    virtual void preprocessing() = 0;
    virtual void postprocessing() = 0;
    // TODO: change return type to what you need
    virtual void step() = 0;
  //public:
};

#endif
