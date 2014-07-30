#include "../htab.h"
#include <string.h>
#include <stdio.h>
struct teststr {
    bool valid;
    const char *what;
};
#define ts_null(ts)(!(ts)->valid)
#define ts_eq(ts, cp) (!strcmp((ts)->what, *(cp)))
#define ts_hash(strp) strlen(*(strp))
DECL_HTAB_KEY(teststr, const char *, struct teststr, ts_hash, ts_eq, ts_null, /*nil_byte*/0);
DECL_HTAB(teststr_int, teststr, int);

int main() {
    HTAB(teststr_int) *hp;
    HTAB_STORAGE(teststr_int) stor;
    htab_storage_init(&stor, teststr_int);
    hp = &stor.h;
    htab_set(hp, "foo", 53, teststr_int);

}
