#include "util/alstack.h"
#include <stdio.h>

int main() {
   struct alstack stk = alstack_new();
   for (int i = 0; i < 10; i++) {
      memset(alstack_alloc(&stk, 500), i, 500);
   }
   memset(alstack_alloc(&stk, 100000), 0xde, 100000);
   void *tell = alstack_tell(&stk);
   for (int i = 0; i < 10000; i++) {
      *(char *) alstack_alloc(&stk, 1000000) = 123;
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
