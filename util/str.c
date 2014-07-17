#include "str.h"

size_t str_cpy(str *str, const char *restrict cstr) {
   size_t len = strlen(cstr);
   vec_resize(str, len);
   memcpy(str->base, cstr, len);
   return len;
}

int str_cpyf(str *str, const char *restrict format, ...) {
   va_list ap;
   va_start(ap, format);
   int ret = str_vcpyf(str, format, ap);
   va_end(ap);
   return ret;
}

int str_vcpyf(str *str, const char *restrict format, va_list ap) {
   vec_clear(str);
   return str_vcatf(str, format, ap);
}

size_t str_cat(str *str, const char *restrict cstr) {
   size_t len2 = strlen(cstr);
   size_t len1 = str->length;
   vec_resize(str, safe_add(len1, len2));
   memcpy(vgetp(str, len1), cstr, len2);
   return len2;
}

int str_catf(str *str, const char *restrict format, ...) {
   va_list ap;
   va_start(ap, format);
   int ret = str_vcatf(str, format, ap);
   va_end(ap);
   return ret;
}

int str_vcatf(str *str, const char *restrict format, va_list ap) {
   // optimize?
   size_t have = str->capacity - str->length;
   int len = vsnprintf(str->base + str->length, have, format, ap);
   if (len < 0)
      return len;
   size_t orig_len = str->length;
   vec_resize(str, orig_len + len);
   if ((size_t) len > have) {
      return vsnprintf(str->base + orig_len, str->capacity - str->length, format, ap);
   } else
      return len;
}
