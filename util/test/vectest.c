#include "util/vecs.h"
#include <stdio.h>

__attribute__((noinline))
int foo(VEC(int) *vi) {
   vec_resize(vi, 6);
   vset(vi, 5, 4);
   return vget(vi, 5);
}

int main() {
   VEC(int) *vi;
   vec_stackalloc(&vi, int);
   printf("%d\n", foo(vi));
   VEC_STORAGE(int, 5) vi2;
   vec_storage_init(&vi2);
   printf("%d\n", foo(&vi2.v));

   vec_zero(&vi2.v);
   VEC(int) *vi2p = &vi2.v;
   vset(vi2p, 5, 3);
   printf("len = %zd\n", vi2.v.length);
   vec_foreach(({ printf("Hi\n"); vi2p; }), i, int n) {
      printf("%zd -> %d\n", i, n);
   }
   size_t i;
   vec_foreach_nodecl(vi2p, i, int n) {
      break;
   }

   vec_free(vi2p);
   vec_free(vi);
}

/*
expect-output<<
4
4
len = 6
Hi
0 -> 0
1 -> 0
2 -> 0
3 -> 0
4 -> 0
5 -> 3
>>
expect-exit 0
*/
