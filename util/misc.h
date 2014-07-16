#pragma once
#include <stdlib.h>

static inline size_t safe_mul(size_t a, size_t b) {
   size_t res = a * b;
   if (res / b != a)
      abort();
   return res;
}

static inline size_t safe_add(size_t a, size_t b) {
   size_t res = a + b;
   if (res < a)
      abort();
   return res;
}
static inline size_t _min(size_t a, size_t b) {
   return a < b ? a : b;
}

static inline size_t _max(size_t a, size_t b) {
   return a > b ? a : b;
}

#define LET_LOOP__(expr, ctr) \
   if (0) \
      __done_##ctr: continue; \
   else if (0) \
      __break_##ctr: break; \
   else \
      for (; ; ({ goto __break_##ctr; })) \
         for (expr; ; ({ goto __done_##ctr; }))

#define LET_LOOP_(expr, ctr) LET_LOOP__(expr, ctr)
#define LET_LOOP(expr) LET_LOOP_(expr, __COUNTER__)

#define LET__(expr, ctr) \
   if (0) \
      __done_##ctr: ; \
   else \
      for (expr; ; ({ goto __done_##ctr; }))

#define LET_(expr, ctr) LET__(expr, ctr)
#define LET(expr) LET_(expr, __COUNTER__)

/*
   usage: defer { foo };
*/

#define defer defer_(__COUNTER__)
#define defer_(ctr) defer__(ctr)
#ifdef __clang__
__attribute__((always_inline))
static inline void defer_cleanup(void (^*cb)()) {
   (*cb)();
}
#define defer__(ctr) \
   __attribute__((cleanup(defer_cleanup))) \
   void (^__defer_##ctr)() = ^()
#else
#define defer__(ctr) \
   __attribute__((always_inline)) \
   auto inline void __defer_func_##ctr(int *); \
   __attribute__((cleanup(__defer_func_##ctr))) \
   int __defer_dummy_##ctr; \
   void __defer_func_##ctr(int *__defer_dummy)
#endif

struct allocator {
   void *(*realloc_func)(void *ptr, size_t oldsize, size_t size, void *ctx);
   void *ctx;
};
