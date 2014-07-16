#pragma once
#include "vecs.h"
#include <stdarg.h>

typedef VEC(char) str;
#define STR_DEFAULT_CAPA 64
#define str_stackalloc(strp) vec_stackalloc_capa(strp, char, STR_DEFAULT_CAPA)

size_t str_cpy(str *str, const char *restrict cstr);
int str_cpyf(str *str, const char *restrict format, ...);
int str_vcpyf(str *str, const char *restrict format, va_list ap);

size_t str_cat(str *str, const char *restrict cstr);
int str_catf(str *str, const char *restrict format, ...);
int str_vcatf(str *str, const char *restrict format, va_list ap);

#define STRF(str) (int) (str)->length, (str)->base
#define str_free vec_free
