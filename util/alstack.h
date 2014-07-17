#pragma once
#include <stdlib.h>
#include <string.h>
#define ALSTACK_ALIGN 7

struct alstack_chunk_hdr {
   void *begin;
   struct alstack_chunk_hdr *prev;
};

struct alstack {
   void *chunk_cur;
   struct alstack_chunk_hdr *chunk_end;
};

struct alstack alstack_new();
void alstack_del(struct alstack *stk);
static inline void *alstack_alloc(struct alstack *stk, size_t size);
static inline void *alstack_zalloc(struct alstack *stk, size_t size);
static inline void *alstack_tell(const struct alstack *stk);
void alstack_pop_to(struct alstack *stk, void *tell);
void *_alstack_alloc_expand(struct alstack *stk, size_t size);
static inline struct allocator alstack_al(struct alstack *stk);

static inline void *alstack_tell(const struct alstack *stk) {
   return stk->chunk_cur;
}

static inline void *alstack_alloc(struct alstack *stk, size_t size) {
   if (size > (size_t) ((char *) stk->chunk_end - (char *) stk->chunk_cur)) {
      return _alstack_alloc_expand(stk, size);
   }
   void *ret = stk->chunk_cur;
   size = (size + ALSTACK_ALIGN) & ~ALSTACK_ALIGN;
   stk->chunk_cur = (char *) stk->chunk_cur + size;
   return ret;
}

static inline void *alstack_zalloc(struct alstack *stk, size_t size) {
   void *ptr = alstack_alloc(stk, size);
   memset(ptr, 0, size);
   return ptr;
}

#include "misc.h"
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
