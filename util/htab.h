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

static inline void htab_free_internal(struct htab_internal *hi) {
   if (hi->base != hi->hi_storage)
      free(hi->base);
}

#define UNUSED_STATIC_INLINE __attribute__((unused)) static inline

#define DECL_HTAB_KEY( \
    name, \
    /* The key type. */ \
    key_ty \
) \
    DO_DECL_HTAB_KEY(name, key_ty, ) \
    typedef char __htab_decl_want_semicolon_here_##name

/* Declare and impl in one - see IMPL_HTAB_KEY */
#define DECL_STATIC_HTAB_KEY( \
    name, \
    key_ty, \
    hash_func, \
    eq_func, \
    null_func, \
    nil_byte \
) \
    DO_DECL_HTAB_KEY(name, key_ty, UNUSED_STATIC_INLINE) \
    IMPL_HTAB_KEY(name, hash_func, eq_func, null_func, nil_byte)

#define DO_DECL_HTAB_KEY(name, key_ty, func_decl) \
    typedef typeof(key_ty) __htab_key_key_ty_##name; \
    DO_DECL_HTAB_KEY_(name, __htab_key_key_ty_##name, func_decl)
#define DO_DECL_HTAB_KEY_(name, key_ty, func_decl) \
    func_decl void *__htab_key_lookup_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        bool add); \
    func_decl bool __htab_key_remove_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size); \
    func_decl void __htab_key_memset_##name( \
        void *ptr, \
        size_t size); \
    func_decl bool __htab_key_is_null_##name( \
        const key_ty *bucket); \
    func_decl void __htab_key_resize_##name( \
        struct htab_internal *restrict hi, \
        size_t size, \
        size_t entry_size);

#define IMPL_HTAB_KEY(name, \
    /* size_t hash_func(const ty *) - hash. \
     * May be a macro. */ \
    hash_func, \
    /* bool eq_func(const ty *stored, const ty *queried) - check whether the \
     * stored key is equal to the requested key (which is assumed to be valid). \
     * May be a macro. */ \
    eq_func, \
    /* bool null_func(const ty *) - check whether the stored key is a \
     * special invalid marker. \
     * May be a macro. */ \
    null_func, \
    /* uint8_t nil_byte - which byte to memset to initially. \
     * null_func(<all bytes nil_byte>) must be true. */ \
    nil_byte \
) \
    DO_IMPL_HTAB_KEY(name, __htab_key_key_ty_##name, hash_func, \
                     eq_func, null_func, nil_byte)
#define DO_IMPL_HTAB_KEY(name, key_ty, hash_func, eq_func, null_func, nil_byte) \
    void *__htab_key_lookup_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        bool add) { \
        size_t capacity = hi->capacity; \
        size_t hash = (hash_func(key)) % capacity; \
        size_t i = hash; \
        char *buckets = hi->base; \
        do { \
            key_ty *bucket = (void *) (buckets + i * entry_size); \
            if (null_func(bucket)) { \
                if (add) { \
                    *bucket = *key; \
                    hi->length++; \
                    return bucket; \
                } else { \
                    return NULL; \
                } \
            } \
            if (eq_func(bucket, key)) \
                return bucket; \
        } while ((i = (i + 1) % capacity) != hash); \
        return NULL; \
    } \
    \
    /* slow but who cares */ \
    bool __htab_key_remove_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size) { \
        void *op = __htab_key_lookup_##name(hi, key, entry_size, false); \
        if (!op) \
            return false; \
        key_ty *orig = op; \
        key_ty *end = hi->base + hi->capacity * entry_size; \
        key_ty *cur = orig; \
        cur += entry_size; \
        if (cur == end) \
            cur = hi->base; \
        key_ty *prev = cur; \
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
    void __htab_key_memset_##name( \
        void *ptr, \
        size_t size) { \
        memset(ptr, (nil_byte), size); \
    } \
    bool __htab_key_is_null_##name( \
        const key_ty *bucket) { \
        return null_func(bucket); \
    } \
    void __htab_key_resize_##name( \
        struct htab_internal *restrict hi, \
        size_t size, \
        size_t entry_size) { \
        size_t old_size = hi->capacity * entry_size; \
        size_t new_size = safe_mul(size, entry_size); \
        void *new_buf = malloc(new_size); \
        __htab_key_memset_##name(new_buf, new_size); \
        struct htab_internal temp; \
        temp.length = 0; \
        temp.capacity = size; \
        temp.base = new_buf; \
        for (size_t i = 0; i < old_size; i += entry_size) { \
            key_ty *bucket = (void *) ((char *) hi->base + i); \
            if (!null_func(bucket)) { \
                memcpy( \
                    __htab_key_lookup_##name(&temp, bucket, entry_size, true), \
                    bucket, \
                    entry_size); \
            } \
        } \
        hi->capacity = size; \
        htab_free_internal(hi); \
        hi->base = new_buf; \
    } \
    typedef char __htab_want_semicolon_here_##name

#define DECL_HTAB( \
    name, \
    /* The name parameter to DECL_HTAB_KEY */ \
    key_name, \
    value_ty) \
    typedef __htab_key_key_ty_##key_name __htab_key_ty_##name; \
    typedef typeof(value_ty) __htab_value_ty_##name; \
    \
    DO_DECL_HTAB(name, __htab_key_ty_##name, __htab_value_ty_##name, key_name)

#define DO_DECL_HTAB(name, key_ty, value_ty, key_name) \
    UNUSED_STATIC_INLINE void *__htab_lookup_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size, \
        bool add) { \
        return __htab_key_lookup_##key_name(hi, key, entry_size, add); \
    } \
    UNUSED_STATIC_INLINE bool __htab_remove_##name( \
        struct htab_internal *restrict hi, \
        const key_ty *restrict key, \
        size_t entry_size) { \
        return __htab_key_remove_##key_name(hi, key, entry_size); \
    } \
    UNUSED_STATIC_INLINE void __htab_memset_##name( \
        void *ptr, \
        size_t size) { \
        return __htab_key_memset_##key_name(ptr, size); \
    } \
    UNUSED_STATIC_INLINE bool __htab_is_null_##name( \
        const key_ty *bucket) { \
        return __htab_key_is_null_##key_name(bucket); \
    } \
    UNUSED_STATIC_INLINE void __htab_resize_##name( \
        struct htab_internal *restrict hi, \
        size_t size, \
        size_t entry_size) { \
        return __htab_key_resize_##key_name(hi, size, entry_size); \
    } \
    \
    struct __htab_bucket_##name { \
        key_ty key; \
        value_ty value; \
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
    __htab_memset_##name(__hs->h.base, \
            sizeof(*__hs) - offsetof(typeof(*__hs), h.storage)); \
} while (0)

#define htab_storage_to_htab(hsoh) \
   __builtin_choose_expr( \
      sizeof((hsoh)->h), \
      &(hsoh)->h, \
      (hsoh)) \

#define htab_bucket_to_valuep(bucket, name) ({ \
    struct __htab_bucket_##name *__btv_bucket = (bucket); \
    __btv_bucket ? &__btv_bucket->value : NULL; \
})

#define htab_len(ht) \
    (htab_storage_to_htab(ht)->length)

#define htab_getbucket(ht, key, name) ({ \
    HTAB(name) *__gpht = htab_storage_to_htab(ht); \
    (__htab_bucket_##name *) __htab_lookup_##name( \
        &__gpht->hi, \
        (key), \
        sizeof(struct __htab_bucket_##name), \
        offsetof(struct __htab_bucket_##name, value), \
        false); \
})

#define htab_getp(ht, key, name) \
    htab_bucket_to_valuep(htab_getbucket(ht, key, name), name)


#define htab_setp(ht, key, name) ({ \
    HTAB(name) *__ht = htab_storage_to_htab(ht); \
    typeof(key) __key = (key); \
    __htab_value_ty_##name *__val = htab_setp_noresize(__ht, __key, name); \
    if (!__val) { \
        htab_resize(__ht, (__ht->capacity * 2) + 1, name); \
        __val = htab_setp_noresize(__ht, __key, name); \
    } \
    __val; \
})

#define htab_setbucket(ht, key, name) ({ \
    HTAB(name) *__spht = htab_storage_to_htab(ht); \
    (struct __htab_bucket_##name *) __htab_lookup_##name( \
        &__spht->hi, \
        (key), \
        sizeof(struct __htab_bucket_##name), \
        true); \
})

#define htab_setp_noresize(ht, key, name) \
    htab_bucket_to_valuep(htab_setbucket(ht, key, name), name)

#define htab_get(ht, key, name) \
    (*htab_getp(ht, key, name))
#define htab_set(ht, key, val, name) \
    (*htab_setp(ht, key, name) = (val))

#define htab_resize(ht, size, name) \
    __htab_resize_##name(&htab_storage_to_htab(ht)->hi, size, \
        sizeof(struct __htab_bucket_##name))

#define htab_free(ht, name) \
    htab_free_internal(&htab_storage_to_htab(ht)->hi)

#define htab_foreach(ht, key_var, val_var, name) \
    LET(HTAB(name) *__ht = htab_storage_to_htab(ht)) \
        for (size_t __htfe_bucket = 0; __htfe_bucket < __ht->capacity; __htfe_bucket++) \
            if(__htab_is_null_##name(&__ht->base[__htfe_bucket].key)) \
                continue; \
            else \
                LET_LOOP(key_var = &__ht->base[__htfe_bucket].key) \
                LET_LOOP(val_var = &__ht->base[__htfe_bucket].value)

