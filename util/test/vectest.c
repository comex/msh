#include "../vecs.h"
#include <stdio.h>
DECL_VEC(int, bazzie);

__attribute__((noinline))
int foo(VEC(bazzie) *vi) {
   vec_resize(vi, 6);
   vset(vi, 5, 4);
   return vget(vi, 5);
}

int main() {
   VEC_STORAGE(bazzie) vi;
   vec_storage_init(&vi);
   printf("%d\n", foo(&vi.v));
   VEC_STORAGE_CAPA(bazzie, 5) vi2;
   vec_storage_init(&vi2);
   printf("%d\n", foo(&vi2.v));

   vec_zero(&vi2.v);
   VEC(bazzie) *vi2p = &vi2.v;
   vset(vi2p, 5, 3);
   vec_add_space(vi2p, 4, 3);
   vset(vi2p, 4, 42);
   vec_remove(vi2p, 3, 2);
   printf("len = %zd\n", vi2.v.length);
   vec_foreach(({ printf("Hi\n"); vi2p; }), i, int *n) {
      printf("%zd -> %d\n", i, *n);
   }
   size_t i;
   vec_foreach_noidxdecl(vi2p, i, UNUSED int *n) {
      break;
   }

   vec_free(vi2p);
   vec_free(&vi);
}

#include "../vec.c"

/*
expect-output<<
4
4
len = 7
Hi
0 -> 0
1 -> 0
2 -> 0
3 -> 3
4 -> 0
5 -> 0
6 -> 3
>>
expect-exit 0
*/
