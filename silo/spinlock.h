#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <stdint.h>
#include <stdlib.h>

#include "amd64.h"
#include "macros.h"
#include "util.h"

class spinlock {
public:
  spinlock() : value(0) {}

  spinlock(const spinlock &) = delete;
  spinlock(spinlock &&) = delete;
  spinlock &operator=(const spinlock &) = delete;

  inline void
  lock()
  {
    // XXX: implement SPINLOCK_BACKOFF
    uint32_t v = value;
    // double count = 0;
    // holder++;
    // std::cerr<<"Caling_spinlock_lock"<<std::endl;

    //sync_bool_compare_and_swap cpp memory order (has different levels)
    // try to delete v from the code
    while (v || !__sync_bool_compare_and_swap(&value, 0, 1)) {
    // while ( !__sync_bool_compare_and_swap(&value, 0, 1)) {
      
      nop_pause();
      v = value;
      // if (++count > 1e10){
      //   std::cerr<<std::to_string(holder)+"are_queuing_to_lock_"+util::hexify(this)<<std::endl;
      // }
    // std::cerr<<"spinlock_lock_spinned"+std::to_string(count++)<<std::endl;
    }

    
    COMPILER_MEMORY_FENCE;
  }

  inline bool
  try_lock()
  {
    return __sync_bool_compare_and_swap(&value, 0, 1);
  }

  inline void
  unlock()
  {
    INVARIANT(value);
    value = 0;
    // holder--;
    COMPILER_MEMORY_FENCE;
  }

  // just for debugging
  inline bool
  is_locked() const
  {
    return value;
  }

private:
  volatile uint32_t value;
  // volatile uint16_t holder=0;
};

#endif /* _SPINLOCK_H_ */
