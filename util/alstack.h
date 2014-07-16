#pragma once
#include "util/misc.h"
#include <stdlib.h>
#include <string.h>
#define ALSTACK_ALIGN 7
#define ALSTACK_INIT_SIZE (64*1024)

typedef uint32_t als_off;
#define ALSGET(stk, ptr, ty) \
   ((ty) ((char *) (stk)->begin + (ptr)))

struct alstack {
   void *begin;
   als_off cur;
   als_off size;
};

void alstack_freeze(const struct alstack *stk);
void _alstack_expand(const struct alstack *stk, size_t size);

struct alstack alstack_new() {
   void *alloc = malloc(ALSTACK_SIZE);
   return (struct alstack) { alloc, 0, ALSTACK_SIZE };
}

static inline void *alstack_tell(const struct alstack *stk) {
   return stk->cur;
}

static inline als_off alstack_alloc_unchecked(struct alstack *stk, size_t size) {
   als_off ret = stk->cur;
   size = (size + ALSTACK_ALIGN) & ~ALSTACK_ALIGN;
   stk->cur = ret + size;
   return ret;
}

static inline als_off alstack_alloc(struct alstack *stk, size_t size) {
   if (size > stk->size - stk->cur)
      _alstack_expand(stk, size);
   return alstack_alloc_unchecked(stk, size);
}

static inline als_off alstack_zalloc(struct alstack *stk, size_t size) {
   als_off ptr = alstack_alloc(stk, size);
   memset(ALSGET(stk, ptr, void *), 0, size);
   return ptr;
}

static void alstack_pop_to(struct alstack *stk, als_off tell) {
   ensure(stk->begin <= tell && tell <= stk->end);
   stk->cur = tell;
}

static void alstack_del(struct alstack *stk) {
   free(stk->begin);
}

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
