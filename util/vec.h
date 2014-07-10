#pragma once
#include <stdlib.h>
#include <string.h>

// Once more into the C macros.  So pointless?
struct __vec_marker {};
struct __vec_internal {
   size_t length;
   size_t capacity;
   void *base;
   void *storage[0];
};

static inline size_t safe_mul(size_t a, size_t b) {
   size_t res = a * b;
   if(res / b != a)
      abort();
   return res;
}

static inline void __vec_realloc(struct __vec_internal *vi, size_t n, size_t esize) {
   if(n > vi->capacity && n < vi->capacity * 2)
      n = vi->capacity * 2;
   size_t new_size = safe_mul(n, esize);
   if(vi->base == vi->storage) {
      void *storage = malloc(new_size);
      memcpy(storage, vi->base, vi->length * esize);
      vi->base = storage;
   } else {
      vi->base = realloc(vi->base, new_size);
   }
}

static inline void *__vec_alloc_capa(size_t n, size_t esize) {
   struct __vec_internal *vi = malloc(sizeof(struct __vec_internal));
   void *storage = malloc(safe_mul(n, esize));
   vi->length = 0;
   vi->capacity = n;
   vi->base = storage;
   return vi;
}

static inline void __vec_free(struct __vec_internal *vi) {
   free(vi->base);
   free(vi);
}

#define VEC(ty) __typeof(__typeof(ty) (*)(struct __vec_marker))

#define VEC_MTYPE(vec) __typeof((vec)((struct __vec_marker) {}))

#define vget(vec, n) (*vgetp(vec, n))

#define vset(vec, n, val) ({ \
   typeof(vec) __v = (vec); /* for sequence */ \
   *vgetp(__v, n) = val; \
})

#define vgetp(vec, n) ({ \
   struct __vec_internal *__vi = (void *) (vec); \
   &((VEC_MTYPE(vec) *) __vi->base)[n]; \
})

#define vresize(vec, n) ({ \
   struct __vec_internal *__vi = (void *) (vec); \
   size_t __n = (n); \
   if(__n >= __vi->capacity || (__vi->base != __vi->storage && __n * 2 < __vi->capacity)) \
      __vec_realloc(__vi, __n, sizeof(VEC_MTYPE(vec))); \
   __vi->length = __n; \
   &((VEC_MTYPE(vec) *) __vi->base)[n]; \
})

__attribute__((always_inline))
static inline void __vec_stackalloc_cleanup(void *p) {
   struct __vec_internal *vi = p;
   if(vi->base != vi->storage)
      free(vi->base);
}

#define vec_stackalloc_capa(vec, n) /* can't use do */ \
   __attribute__((cleanup(__vec_stackalloc_cleanup))) \
   struct { \
      struct __vec_internal vi; \
      VEC_MTYPE(vec) storage[n]; \
   } vec##__data; \
   vec##__data.vi.length = 0; \
   vec##__data.vi.capacity = (n); \
   vec##__data.vi.base = vec##__data.vi.storage; \
   vec = (void *) &vec##__data.vi;

#define VEC_DEFAULT_CAPA 5

#define vec_stackalloc(vec) vec_stackalloc_capa(vec, VEC_DEFAULT_CAPA)

#define vec_alloc_capa(ty, n) ((VEC(ty)) __vec_alloc_capa((n), sizeof(ty)))

#define vec_alloc(ty) vec_alloc_capa(ty, VEC_DEFAULT_CAPA)

// heap only
#define vec_free(vec) __vec_free((void *) (VEC_MTYPE(vec) *) (vec))
