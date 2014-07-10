#include "vec.h"
#include <stdio.h>

__attribute__((noinline))
int foo(VEC(int) vi) {
   vresize(vi, 6);
   vset(vi, 5, 4);
   return vget(vi, 5);
}

int main() {
   VEC(int) vi; vec_stackalloc(vi);
   printf("%d\n", foo(vi));
   VEC(int) vi2 = vec_alloc(int);
   printf("%d\n", foo(vi2));

   vec_zero(vi2);
   vset(vi2, 5, 3);
   printf("len = %zd\n", vlen(vi2));
   vec_foreach(vi2, i, int n) {
      printf("%zd -> %d\n", i, n);
   }
   size_t i;
   vec_foreach_nodecl(vi2, i, int n) {
      break;
   }

   vec_free(vi2);
}

/*
expect-output<<
4
4
len = 6
0 -> 0
1 -> 0
2 -> 0
3 -> 0
4 -> 0
5 -> 3
>>
expect-exit 0
*/
