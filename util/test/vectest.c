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
   vec_free(vi2);
}
