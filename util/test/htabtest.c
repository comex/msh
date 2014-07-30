#include "../htab.h"
#include <string.h>
#include <stdio.h>
struct teststr {
    bool valid;
    const char *what;
};
#define ts_null(ts) ({ if (0) printf("null? %p\n", *ts); !*ts; })
#define ts_eq(ts, cp) ({ if (0) printf("eq? %p %p\n", *ts, *cp); !strcmp(*(ts), *(cp)); })
#define ts_hash(strp) strlen(*(strp))
DECL_HTAB_KEY(teststr, const char *);
DECL_HTAB(teststr_int, teststr, int);

int main() {
    HTAB(teststr_int) *hp;
    HTAB_STORAGE(teststr_int) stor;
    htab_storage_init(&stor, teststr_int);
    hp = &stor.h;
    for(int i = 0; i < 100; i++) {
        const char *k;
        asprintf((char **) &k, "foo%d", i);
        htab_set(hp, &k, i, teststr_int);
    }
    htab_foreach(hp, const char **k, int *v, teststr_int) {
        if(*v % 10 == 1)
            printf("%s -> %d\n", *k, *v);
    }
    htab_free(hp, teststr_int);
}

/*
expect-output<<
foo91 -> 91
foo21 -> 21
foo1 -> 1
foo11 -> 11
foo31 -> 31
foo41 -> 41
foo51 -> 51
foo61 -> 61
foo71 -> 71
foo81 -> 81
>>
expect-exit 0
*/

IMPL_HTAB_KEY(teststr, ts_hash, ts_eq, ts_null, /*nil_byte*/0);
