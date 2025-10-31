// problem is: configure rtx so threads/timers run predictably with minimal noise.
// solution: enable systick as kernel timer, set fixed tick and small stacks, keep user timers on,
//           disable round-robin, and provide minimal idle/error hooks. the rest is left default.

#include "cmsis_os.h"

/* core thread config */
#ifndef OS_TASKCNT
#define OS_TASKCNT       6      // max concurrent user threads
#endif
#ifndef OS_STKSIZE
#define OS_STKSIZE       50     // default stack (words)
#endif
#ifndef OS_MAINSTKSIZE
#define OS_MAINSTKSIZE   50     // main thread stack (words)
#endif
#ifndef OS_STKCHECK
#define OS_STKCHECK      1      // stack overflow checking
#endif
#ifndef OS_RUNPRIV
#define OS_RUNPRIV       0      // unprivileged threads
#endif

/* kernel tick using systick */
#ifndef OS_SYSTICK
#define OS_SYSTICK       1
#endif
#ifndef OS_CLOCK
#define OS_CLOCK         10000000
#endif
#ifndef OS_TICK
#define OS_TICK          10000  // us per tick (10 ms)
#endif

/* scheduling */
#ifndef OS_ROBIN
#define OS_ROBIN         0      // no round-robin
#endif

/* user timers */
#ifndef OS_TIMERS
#define OS_TIMERS        1
#endif
#ifndef OS_TIMERPRIO
#define OS_TIMERPRIO     5      // high
#endif
#ifndef OS_TIMERSTKSZ
#define OS_TIMERSTKSZ    50     // words
#endif
#ifndef OS_TIMERCBQS
#define OS_TIMERCBQS     4
#endif

/* isr fifo + c library mutexes */
#ifndef OS_FIFOSZ
#define OS_FIFOSZ        16
#endif
#ifndef OS_MUTEXCNT
#define OS_MUTEXCNT      8
#endif

/* systick reload value */
#define OS_TRV ((uint32_t)(((double)OS_CLOCK*(double)OS_TICK)/1e6) - 1)

/* idle hook: runs when nothing else is ready */
void os_idle_demon (void) {
  for (;;) { /* idle */ }
}

/* error hook: trap on runtime errors */
void os_error (uint32_t error_code) {
  (void)error_code;
  for (;;) { /* halt */ }
}

#include "RTX_CM_lib.h"
