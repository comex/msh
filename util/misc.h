#pragma once
#include <stdlib.h>

static inline size_t safe_mul(size_t a, size_t b) {
   size_t res = a * b;
   if(res / b != a)
      abort();
   return res;
}

#define LET__(expr, ctr) \
   if(0) \
      done_##ctr: continue; \
   else if(0) \
      break_##ctr: break; \
   else \
      for(; ; ({ goto break_##ctr; })) \
         for(expr; ; ({ goto done_##ctr; }))

#define LET_(expr, ctr) LET__(expr, ctr)
#define LET(expr) LET_(expr, __COUNTER__)
