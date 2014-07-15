#include "alstack.h"
#define ALSTACK_MIN_CHUNK (1024 - sizeof(struct alstack_chunk_hdr))
#define ALSTACK_MAX_CHUNK (10*1024*1024)

static void *alstack_expand(struct alstack *stk, size_t size) {
   if (size < ALSTACK_MIN_CHUNK)
      size = ALSTACK_MIN_CHUNK;
   else if (size > ALSTACK_MAX_CHUNK)
      return NULL;
   size = (size + ALSTACK_ALIGN) & ~ALSTACK_ALIGN;
   void *stuff = malloc(size + sizeof(struct alstack_chunk_hdr));
   struct alstack_chunk_hdr *hdr = (void *) ((char *) stuff + size);
   hdr->prev = stk->chunk_end;
   hdr->begin = stuff;
   stk->chunk_cur = stuff;
   stk->chunk_end = hdr;
   return stuff;
}

struct alstack alstack_new() {
   struct alstack stk = { NULL, NULL };
   alstack_expand(&stk, 1);
   return stk;
}

void *_alstack_alloc_expand(struct alstack *stk, size_t size) {
   void *ret = alstack_expand(stk, size);
   if (!ret)
      return NULL;
   size = (size + ALSTACK_ALIGN) & ~ALSTACK_ALIGN;
   stk->chunk_cur = (char *) ret + size;
   return ret;
}

static void alstack_pop(struct alstack *stk) {
   struct alstack_chunk_hdr *prev = stk->chunk_end->prev;
   free(stk->chunk_end->begin);
   stk->chunk_end = prev;
}

void alstack_pop_to(struct alstack *stk, void *tell) {
   uintptr_t tell_u = (uintptr_t) tell;
   // careful about undefined behavior!
   while (!((uintptr_t) stk->chunk_end->begin <= tell_u && tell_u <= (uintptr_t) stk->chunk_end))
      alstack_pop(stk);
   stk->chunk_cur = tell;
}

void alstack_del(struct alstack *stk) {
   while (stk->chunk_end)
      alstack_pop(stk);
   stk->chunk_cur = NULL;
}
