#include "../event.h"
#include "../misc.h"
#include <stdio.h>
#include <unistd.h>

static int handler(int fd, short revents, void *ctx) {
   ensure(ctx == (void *) 0xdeadbeef);
   ensure(revents & POLLIN);
   return -4;
}

int main() {
   int fds[2];
   ensure(!pipe(fds));
   struct event_loop *el = event_loop_new();
   event_loop_add_fd(el, fds[0], POLLIN, handler, (void *) 0xdeadbeef);
   ensure(write(fds[1], "a", 1) == 1);
   ensure(event_loop_poll(el) == -4);
   ensure(event_loop_remove_fd(el, fds[0]) == (void *) 0xdeadbeef);
   event_loop_free(el);
   return 0;
}

#include "../event.c"

/*
expect-output<<
>>
expect-exit 0
*/
