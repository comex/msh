#include "alstack.h"

static void *alstack_free_list;

struct alstack alstack_new() {
   // 64MB is probably too big for allocators to bother with
   void *alloc;
   if (!alstack_free_list) {
      alloc = malloc(ALSTACK_SIZE);
   } else {
      alloc = alstack_free_list;
      alstack_free_list = *(void **) alloc;
   }
   return (struct alstack) { alloc, alloc, (char *) alloc + ALSTACK_SIZE };
}

void alstack_del(struct alstack *stk) {
   void *alloc = stk->begin;
   *(void **) alloc = alstack_free_list;
   alstack_free_list = alloc;
   stk->begin = stk->cur = stk->end = NULL;

}
