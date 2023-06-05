#ifndef __EASY_SCHEDULER_H__
#define __EASY_SCHEDULER_H__

#include "easy/base/noncopyable.h"

namespace easy {

// N-M asymmetic Scheduler
class Scheduler : noncopyable 
{
public:
  Scheduler();
  virtual ~Scheduler();
  bool stop_;
  private:

};

}

#endif
