#pragma once
#include "util/misc.h"
#include <stdlib.h>
#include <string.h>
#define ALSTACK_ALIGN 7
#define ALSTACK_SIZE (64*1024*1024)

struct alstack {
   void *begin;
   void *cur;
   void *end;
};

struct alstack alstack_new();

static inline void *alstack_tell(const struct alstack *stk) {
   return stk->cur;
}

static inline void *alstack_alloc_unchecked(struct alstack *stk, size_t size) {
   void *ret = stk->cur;
   size = (size + ALSTACK_ALIGN) & ~ALSTACK_ALIGN;
   stk->cur = (char *) ret + size;
   return ret;
}

static inline void *alstack_alloc(struct alstack *stk, size_t size) {
   if (size > (char *) stk->end - (char *) stk->cur)
      abort();
   return alstack_alloc_unchecked(stk, size);
}

static inline void *alstack_zalloc(struct alstack *stk, size_t size) {
   void *ptr = alstack_alloc(stk, size);
   memset(ptr, 0, size);
   return ptr;
}

static void alstack_pop_to(struct alstack *stk, void *tell) {
   ensure(stk->begin <= tell && tell <= stk->end);
   stk->cur = tell;
}

void alstack_del(struct alstack *stk);

static inline void *_alstack_realloc_func(void *ptr, size_t oldsize, size_t size, void *ctx) {
   struct alstack *stk = ctx;
   void *new = alstack_alloc(stk, size);
   if (ptr)
      memcpy(new, ptr, _min(oldsize, size));
   return new;
}

static inline struct allocator alstack_al(struct alstack *stk) {
   return (struct allocator) {_alstack_realloc_func, stk};
}
