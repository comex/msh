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

#define DECL_VEC(ty) \
   union __VEC_##ty { \
      struct vec_internal vi; \
      struct { \
         size_t length; \
         size_t capacity; \
         ty *base; \
         ty storage[1]; \
      }; \
   }

#define VEC(ty) union __VEC_##ty

static inline void vec_realloc_internal(struct vec_internal *vi, size_t n, size_t esize) {
   if (n > vi->capacity && n < vi->capacity * 2)
      n = vi->capacity * 2;
   size_t new_size = safe_mul(n, esize);
   if (vi->base == vi->vi_storage) {
      void *storage = malloc(new_size);
      memcpy(storage, vi->base, vi->length * esize);
      vi->base = storage;
   } else {
      vi->base = realloc(vi->base, new_size);
   }
}

static inline void vec_free_internal(struct vec_internal *vi) {
   if (vi->base != vi->vi_storage)
      free(vi->base);
}

#define VEC_STORAGE(ty, n) \
   struct { \
      VEC(ty) v; \
      typeof(ty) rest[(n)-1]; \
   }

#define vec_storage_init(vs) do { \
   (vs)->v.length = 0; \
   (vs)->v.capacity = (sizeof((vs)->rest) / sizeof((vs)->rest[0])) + 1; \
   (vs)->v.base = (vs)->v.storage; \
} while (0)

#define vec_stackalloc_capa(vec, ty, n) /* can't use do */ \
   VEC_STORAGE(ty, n) vec##__storage; \
   vec_storage_init(&vec##__storage); \
   vec = &vec##__storage.v

#define VEC_DEFAULT_CAPA 5

#define vec_stackalloc(vec, ty) vec_stackalloc_capa(vec, ty, VEC_DEFAULT_CAPA)

// manipulation

#define vget(vec, n) (*vgetp(vec, n))

#define vset(vec, n, val) (*vgetp(vec, n) = val)

// avoid sequence isseus
#define vgetp(vec, n) ({ \
   size_t _n = (n); \
   &(vec)->base[n]; \
})

#define vresize(vec, n) ({ \
   typeof(vec) __vec = (vec); \
   size_t __n = (n); \
   if (__n >= __vec->capacity || (__vec->base != __vec->storage && __n * 2 < __vec->capacity)) \
      vec_realloc_internal(&__vec->vi, __n, sizeof(__vec->storage[0])); \
   __vec->length = __n; \
   &__vec->base[__n]; \
})

#define vappend(vec, val) ({ \
   typeof(vec) __vec = (vec); \
   size_t __l = __vec->length; \
   vresize(__vec, __l + 1); \
   vset(__vec, __l, val); \
})

#define vec_zero(vec) ({ \
   typeof(vec) __vec = (vec); \
   memset(__vec->base, 0, sizeof(__vec->base[0]) * __vec->length); \
})

#define vec_free(vec) \
   vec_free_internal(&(vec)->vi)

#define vec_foreach(vec, idxvar, valvar) \
   LET(typeof(vec) __vec = vec) \
      for (size_t idxvar = 0; idxvar < __vec->length; idxvar++) \
         LET_LOOP(valvar = vget(__vec, idxvar))

#define vec_foreach_nodecl(vec, idxvar, valvar) \
   LET(typeof(vec) __vec = vec) \
      for (idxvar = 0; idxvar < __vec->length; idxvar++) \
         LET_LOOP(valvar = vget(__vec, idxvar))
