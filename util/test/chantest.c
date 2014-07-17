#include "../chan.h"
#include "../event.h"
#include <stdbool.h>
#include <string.h>

static bool received;

static int handler(struct chanr *cr, void *data, size_t size, void *ctx) {
   (void) cr;
   ensure(ctx == (void *) 0x234);
   ensure(size == 5 && !memcmp(data, "hello", 5));
   free(data);
   received = true;
   return 0;
}

int main() {
   struct chanr cr = chanr_new();
   struct event_loop *el = event_loop_new();
   event_loop_add_chanr(el, &cr, handler, (void *) 0x123);
   ensure(event_loop_remove_chanr(el, &cr) == (void *) 0x123);
   event_loop_add_chanr(el, &cr, handler, (void *) 0x234);

   struct chanw cw = chanr_make_writer(&cr);
   chanw_send(&cw, "hello", 5);
   while (!event_loop_poll(el))
      ;
   ensure(received);
   ensure(event_loop_remove_chanr(el, &cr) == (void *) 0x234);
   chanr_del(&cr);
   chanw_del(&cw);
   return 0;
}

#include "../chan.c"
#include "../event.c"

/*
expect-output<<
>>
expect-exit 0
*/
