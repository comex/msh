#pragma once
#include "vecs.h"
#include <stdarg.h>

typedef VEC(char) str;
#define str_stackalloc_capa(strp, capa) \
    str_stackalloc_capa_(strp, capa, __COUNTER__)
#define str_stackalloc_capa_(strp, capa, ctr) \
    str_stackalloc_capa__(strp, capa, ctr)
#define str_stackalloc_capa__(strp, capa, ctr) \
    VEC_STORAGE_CAPA(char, capa) __str_stackalloc_##ctr; \
    vec_storage_init(&__str_stackalloc_##ctr); \
    *(strp) = &__str_stackalloc_##ctr.v;

#define str_stackalloc(strp) \
    str_stackalloc_capa(strp, 64)

size_t str_cpy(str *str, const char *restrict cstr);
int str_cpyf(str *str, const char *restrict format, ...);
int str_vcpyf(str *str, const char *restrict format, va_list ap);

size_t str_cat(str *str, const char *restrict cstr);
int str_catf(str *str, const char *restrict format, ...);
int str_vcatf(str *str, const char *restrict format, va_list ap);

#define STRF(str) (int) (str)->length, (str)->base
#define str_free vec_free
