#include "alstack.h"

void alstack_freeze(const struct alstack *stk) {
   // XXX
}

void _alstack_expand(const struct alstack *stk, size_t size) {
   size_t new_size = _max(stk->size * 2, safe_add(stk->size, size));
   if (new_size >= 0x100000000)
      abort();
   void *new = realloc(stk->begin, new_size);
   if (!new)
      abort();
   stk->size = new_size;
}
