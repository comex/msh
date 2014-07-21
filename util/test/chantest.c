#include "../chan.h"
#include "../event.h"
#include <stdbool.h>
#include <string.h>

static bool received;

static int handler(struct chanr *cr, void *data, void *ctx) {
   (void) cr;
   ensure(ctx == (void *) 0x234);
   ensure(!strcmp(data, "hello"));
   received = true;
   return 0;
}

int main() {
   struct chanr cr;
   chanr_init(&cr);
   struct event_loop *el = event_loop_new();
   event_loop_add_chanr(el, &cr, handler, (void *) 0x123);
   ensure(event_loop_remove_chanr(el, &cr) == (void *) 0x123);
   event_loop_add_chanr(el, &cr, handler, (void *) 0x234);

   struct chanw cw = chanr_make_writer(&cr);
   chanw_send(&cw, "hello");
   while (!event_loop_poll(el))
      ;
   ensure(received);
   ensure(event_loop_remove_chanr(el, &cr) == (void *) 0x234);
   chanw_del(&cw);
   chanr_del(&cr);
   return 0;
}

#include "../chan.c"
#include "../event.c"

/*
expect-output<<
>>
expect-exit 0
*/
