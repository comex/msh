#include "util/time.h"
#include <unistd.h>

int main() {
   uint64_t a = monotonic_time_us();
   usleep(10);
   uint64_t b = monotonic_time_us();
   ensure(b > a + 5);
}

#include "util/time.c"

/*
expect-exit 0
*/
