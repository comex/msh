#pragma once
#include "misc.h"
#include <stddef.h>

enum htab_lookup_mode {
    HTAB_MODE_LOOKUP,
    HTAB_MODE_ADD,
    HTAB_MODE_REMOVE
};

struct htab_internal {
    size_t length;
    size_t capacity;
    void *base;
    char hi_storage[1]; // see vec.h
};

#define UNUSED_STATIC_INLINE __attribute__((unused)) static inline

#define DECL_HTAB_KEY( \
    name, \
    /* The key type used in the client API */ \
    key_key_ty, \
    /* The type stored - may need something additional to indicate whether \
     * there is a valid key there (per null_func), or may not (e.g. any \
     * pointer type key can use NULL as a special value). */ \
    key_storage_ty, \
    /* size_t hash_func(const ty *) - hash. \
     * May be a macro. */ \
    hash_func, \
    /* bool eq_func(const key_storage_ty *, const ty *) - check whether the stored \
     * key is equal to the requested key (which is assumed to be valid). \
     * May be a macro. */ \
    eq_func, \
    /* bool null_func(const key_storage_ty *) - check whether the stored key is a \
     * special invalid marker. \
     * May be a macro. */ \
    null_func, \
    /* uint8_t nil_byte - which byte to memset to initially. \
     * null_func(<all bytes nil_byte>) must be true. */ \
    nil_byte \
) \
    typedef typeof(key_key_ty) __htab_key_key_ty_##name; \
    typedef typeof(key_storage_ty) __htab_key_storage_ty_##name; \
    enum { __htab_key_nil_byte_##name = (nil_byte) }; \
    \
    DO_DECL_HTAB_KEY(name, __htab_key_key_ty_##name, __htab_key_storage_ty_##name, \
        hash_func, eq_func, null_func, \
        UNUSED_STATIC_INLINE)


#define DO_DECL_HTAB_KEY(name, key_ty, storage_ty, \
                         hash_func, eq_func, null_func, \
                         func_decl) \
    func_decl void *__htab_key_lookup_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        size_t value_off, \
        bool add) { \
        size_t capacity = hi->capacity; \
        size_t hash = (hash_func(key)) % capacity; \
        size_t i = hash; \
        char *buckets = hi->base; \
        do { \
            storage_ty *bucket = (void *) (buckets + i * entry_size); \
            if (null_func(bucket)) { \
                if (add) { \
                    hi->length++; \
                    return (char *) bucket + value_off; \
                } else { \
                    return NULL; \
                } \
            } \
            if (eq_func(bucket, key)) \
                return (char *) bucket + value_off; \
        } while ((i = (i + 1) % capacity) != hash); \
        return NULL; \
    } \
    \
    /* slow but who cares */ \
    func_decl bool __htab_key_remove_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        size_t value_off) { \
        void *op = __htab_key_lookup_##name(hi, key, entry_size, value_off, false); \
        if (!op) \
            return false; \
        storage_ty *orig = (void *) ((char *) op - value_off); \
        storage_ty *end = hi->base + hi->capacity * entry_size; \
        storage_ty *cur = orig; \
        cur += entry_size; \
        if (cur == end) \
            cur = hi->base; \
        storage_ty *prev = cur; \
        do { \
            memmove(prev, cur, entry_size); \
            cur += entry_size; \
            if (cur == end) \
                cur = hi->base; \
        } while (cur != orig); \
        memset(cur, 0, entry_size); \
        hi->length--; \
        return true; \
    } \
    typedef char __htab_want_semicolon_here_##name

#define DECL_HTAB( \
    name, \
    /* The name parameter to DECL_HTAB_KEY */ \
    key_name, \
    value_ty) \
    typedef __htab_key_key_ty_##key_name __htab_key_ty_##name; \
    typedef __htab_key_storage_ty_##key_name __htab_storage_ty_##name; \
    enum { __htab_nil_byte_##name = __htab_key_nil_byte_##key_name }; \
    typedef typeof(value_ty) __htab_value_ty_##name; \
    \
    DO_DECL_HTAB(name, __htab_key_key_ty_##key_name, \
                 __htab_key_storage_ty_##key_name, key_name) \

#define DO_DECL_HTAB(name, key_ty, storage_ty, key_name) \
    UNUSED_STATIC_INLINE void *__htab_lookup_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        size_t value_off, \
        bool add) { \
        return __htab_key_lookup_##key_name(hi, key, entry_size, value_off, add); \
    } \
    UNUSED_STATIC_INLINE bool __htab_remove_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        size_t value_off) { \
        return __htab_key_remove_##key_name(hi, key, entry_size, value_off); \
    } \
    \
    struct __htab_bucket_##name { \
        key_ty key; \
        storage_ty value; \
    }; \
    union __HTAB_##name { \
        char h[0]; \
        struct htab_internal hi; \
        struct { \
            size_t length; \
            size_t capacity; \
            struct __htab_bucket_##name *base; \
            struct __htab_bucket_##name storage[1]; \
        }; \
    }

// this stuff is based on vec.h

#define HTAB(name) union __HTAB_##name

#define HTAB_STORAGE_CAPA(name, n) \
   struct { \
      HTAB(name) h; \
      struct __htab_bucket_##name rest[(n)-1]; \
   }

#define HTAB_STORAGE(name) \
    HTAB_STORAGE_CAPA(name, 5)

#define htab_storage_init(hs, name) do { \
    typeof(hs) __hs = (hs); \
    __hs->h.length = 0; \
    __hs->h.capacity = (sizeof(__hs->rest) / sizeof(__hs->rest[0])) + 1; \
    __hs->h.base = __hs->h.storage; \
    memset(__hs->h.base, __htab_nil_byte_##name, \
            sizeof(*__hs) - offsetof(typeof(*__hs), h.storage)); \
} while (0)

#define htab_storage_to_htab(hsoh) \
   __builtin_choose_expr( \
      sizeof((hsoh)->h), \
      &(hsoh)->h, \
      (hsoh) \
   )

#define htab_len(ht) \
    (htab_storage_to_htab(ht)->length)

#define htab_getbucket(ht, key, name) ({ \
    char *__p = (char *) htab_getp(ht, key, name); \
    __p ? (__p - offsetof(struct __htab_bucket_##name, value)) : NULL; \
})

#define htab_getp(ht, key, name) ({ \
    typeof(htab_storage_to_htab(ht)) __gpht = htab_storage_to_htab(ht); \
    (__htab_value_ty_##name *) __htab_lookup_##name( \
        &__gpht->hi, \
        (key), \
        sizeof(struct __htab_bucket_##name), \
        offsetof(struct __htab_bucket_##name, value), \
        false); \
})

#define htab_setp(ht, key, name) ({ \
    typeof(htab_storage_to_htab(ht)) __ht = htab_storage_to_htab(ht); \
    typeof(key) __key = (key); \
    __htab_value_ty_##name *__val = htab_setp_noresize(__ht, __key, name); \
    if (!__val) { \
        htab_resize(__ht, (__ht->capacity * 2) + 1); \
        __val = htab_setp_noresize(__ht, __key, name); \
    } \
    __val; \
})

#define htab_setp_noresize(ht, key, name) ({ \
    typeof(htab_storage_to_htab(ht)) __spht = htab_storage_to_htab(ht); \
    (__htab_value_ty_##name *) __htab_lookup_##name( \
        &__spht->hi, \
        (key), \
        sizeof(struct __htab_bucket_##name), \
        offsetof(struct __htab_bucket_##name, value), \
        true); \
})

#define htab_get(ht, key, name) \
    (*htab_getp(ht, key, name))
#define htab_set(ht, key, val, name) \
    (*htab_setp(ht, key, name) = (val))

#define htab_resize(ht, size, name) \
    htab_resize_internal(&htab_storage_to_htab(ht)->hi, size, \
        sizeof(struct __htab_bucket_##name), __htab_nil_byte_##name)

#define htab_free(ht, size, name) \
    htab_free_internal(&htab_storage_to_htab(ht)->hi)
