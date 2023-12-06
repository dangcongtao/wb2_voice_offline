#include "IRtimer.h"
#include "bl_timer.h"

void reset(IRtimer_t *obj) {
#ifndef UNIT_TEST
  obj->start = bl_timer_now_us();
#else
  start = _IRtimer_unittest_now;
#endif
}

uint32_t elapsed(IRtimer_t *obj) 
{
#ifndef UNIT_TEST
  uint32_t now = bl_timer_now_us();
#else
  uint32_t now = _IRtimer_unittest_now;
#endif
  if (obj->start <= now)      // Check if the system timer has wrapped.
    return now - obj->start;  // No wrap.
  else
    return UINT32_MAX - obj->start + now;  // Has wrapped.
}