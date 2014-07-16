#include "time.h"

#ifdef __APPLE__
#include <mach/mach_time.h>
uint64_t monotonic_time_us() {
   static mach_timebase_info_data_t timebase_info;
   if (!timebase_info.denom)
      ensure(KERN_SUCCESS == mach_timebase_info(&timebase_info));
   return mach_absolute_time() / 1000 * timebase_info.numer / timebase_info.denom;
}

#else

#include <time.h>
uint64_t monotonic_time_us() {
   struct timespec ts;
   ensure(!clock_gettime(CLOCK_MONOTONIC_RAW, &ts));
   return (uint64_t) ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

#endif
