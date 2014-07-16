#pragma once
#include "misc.h"
#include <stdlib.h>
#include <string.h>

// Once more into the C macros.  So pointless?
struct vec_internal {
   size_t length;
   size_t capacity;
   void *base;
   char vi_storage[1]; // must be at least one byte so vec_free works correctly
};

#define DECL_VEC(ty, name) \
   union __VEC_##name { \
      char v[0]; \
      struct vec_internal vi; \
      struct { \
         size_t length; \
         size_t capacity; \
         ty *base; \
         ty storage[1]; \
      }; \
   }

#define VEC(name) union __VEC_##name

static inline void vec_realloc_internal(struct vec_internal *vi, size_t n, size_t esize) {
   if (n > vi->capacity && n < vi->capacity * 2)
      n = vi->capacity * 2;
   size_t new_size = safe_mul(n, esize);
   if (vi->base == vi->vi_storage) {
      void *storage = realloc(NULL, new_size);
      memcpy(storage, vi->base, vi->length * esize);
      vi->base = storage;
   } else {
      vi->base = realloc(vi->base, new_size);
   }
   vi->capacity = new_size;
}

static inline void vec_free_internal(struct vec_internal *vi) {
   if (vi->base != vi->vi_storage)
      free(vi->base);
}

#define VEC_TY(ty) typeof(((VEC(ty) *) 0)->storage[0])

#define VEC_STORAGE(ty, n) \
   struct { \
      char is_vec_or_vec_storage[0]; \
      VEC(ty) v; \
      VEC_TY(ty) rest[(n)-1]; \
   }

#define vec_empty() \
   {0, 0, NULL}

#define vec_storage_init(vs) do { \
   (vs)->v.length = 0; \
   (vs)->v.capacity = (sizeof((vs)->rest) / sizeof((vs)->rest[0])) + 1; \
   (vs)->v.base = (vs)->v.storage; \
} while (0)

#define vec_stackalloc_capa(vecp, ty, n) /* can't use do */ \
   VEC_STORAGE(ty, n) vec##__storage; \
   vec_storage_init(&vec##__storage); \
   *(vecp) = &vec##__storage.v

#define VEC_DEFAULT_CAPA 5

#define vec_stackalloc(vecp, ty) vec_stackalloc_capa(vecp, ty, VEC_DEFAULT_CAPA)

// pointer to (vec storage, vec) -> pointer to vec
#define vec_storage_to_vec(vsov) \
   __builtin_choose_expr( \
      sizeof((vsov)->v), \
      &(vsov)->v, \
      (vsov) \
   )

// manipulation

#define vlen(vec) (vec_storage_to_vec(vec)->length)

#define vget(vec, n) (*vgetp(vec, n))

#define vset(vec, n, val...) (*vgetp(vec, n) = (val))

// avoid sequence isseus
#define vgetp(vec, n) ({ \
   size_t _n = (n); \
   &vec_storage_to_vec(vec)->base[n]; \
})

#define vec_resize(vec, n) ({ \
   typeof(vec_storage_to_vec(vec)) __vec_vr = vec_storage_to_vec(vec); \
   size_t __n = (n); \
   if (__n >= __vec_vr->capacity || (__vec_vr->base != __vec_vr->storage && __n < __vec_vr->capacity / 2)) \
      vec_realloc_internal(&__vec_vr->vi, __n, sizeof(__vec_vr->storage[0])); \
   __vec_vr->length = __n; \
   &__vec_vr->base[__n]; \
})

#define vappend(vec, val...) ({ \
   typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec); \
   size_t __l = __vec->length; \
   vec_resize(__vec, __l + 1); \
   vset(__vec, __l, val); \
})

#define vec_concat(vec, vec2) ({ \
   typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec), __vec2 = vec_storage_to_vec(vec2); \
   size_t __l = __vec->length; \
   vec_resize(__vec, safe_add(__l, __vec2->length)); \
   memcpy(vgetp(__vec, __l), __vec2->base, __vec2->length); \
})

#define vec_zero(vec) ({ \
   typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec); \
   memset(__vec->base, 0, sizeof(__vec->base[0]) * __vec->length); \
})

#define vec_free(vec) \
   vec_free_internal(&vec_storage_to_vec(vec)->vi)

#define vec_clear(vec) \
   vec_resize(vec, 0)

// (guaranteed to *not* cache vec->length)

#define vec_foreach(vec, idxvar, ptrvar) \
   LET(typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec)) \
      for (size_t idxvar = 0; idxvar < __vec->length; idxvar++) \
         LET_LOOP(ptrvar = vgetp(__vec, idxvar))

#define vec_foreach_noidxdecl(vec, idxvar, ptrvar) \
   LET(typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec)) \
      for (idxvar = 0; idxvar < __vec->length; idxvar++) \
         LET_LOOP(ptrvar = vgetp(__vec, idxvar))

#define vec_borrow(ty, ptr, n) ({ \
   size_t __n = (n); \
   ((VEC(ty)) {{ __n, __n, (ptr) }}); \
})

#define vec_add_space(vec, idx, num) ({ \
   typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec); \
   size_t __idx = (idx), __num = (num); \
   size_t __orig = __vec->length; \
   vec_resize(__vec, safe_add(__orig, __num)); \
   memmove(vgetp(__vec, __idx + __num), vgetp(__vec, __idx), (__orig - __idx) * sizeof(__vec->base[0])); \
})

#define vec_remove(vec, idx, num) ({ \
   typeof(vec_storage_to_vec(vec)) __vec = vec_storage_to_vec(vec); \
   size_t __idx = (idx), __num = (num); \
   size_t __orig = __vec->length; \
   memmove(vgetp(__vec, __idx), vgetp(__vec, __idx + __num), (__orig - (__idx + __num)) * sizeof(__vec->base[0])); \
   vec_resize(__vec, __orig - __num); \
})
