#include "util/alstack.h"
#include <stdio.h>

int main() {
   struct alstack stk = alstack_new();
   for (int i = 0; i < 10; i++) {
      als_off buf = alstack_alloc(&stk, 500);
      memset(ALSGET(&stk, buf), i, 500);
   }
   memset(ALSGET(&stk, alstack_alloc(&stk, 100000), void *), 0xde, 100000);
   als_off tell = alstack_tell(&stk);
   for (int i = 0; i < 10000; i++) {
      *ALSGET(&stk, alstack_alloc(&stk, 1000000), char *) = 123;
      if (i % 3 == 0)
         alstack_pop_to(&stk, tell);
   }

   struct allocator al = alstack_al(&stk);
   char *ptr = al.realloc_func(NULL, 0, 10, al.ctx);
   ptr[5] = 'a';
   ptr = al.realloc_func(ptr, 10, 9, al.ctx);
   ptr = al.realloc_func(ptr, 9, 100, al.ctx);
   printf("%c\n", ptr[5]);

   alstack_del(&stk);


}

#include "util/alstack.c"

/*
expect-output<<
a
>>
expect-exit 0
*/
