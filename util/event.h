#pragma once
#include "misc.h"
#include <poll.h>

// this is just a wrapper around poll() - very tiny subset of what libuv does

struct event_loop;
// return < 0 to interrupt
typedef int (*event_loop_fd_handler)(int fd, short revents, void *ctx);

struct event_loop *event_loop_new();
void event_loop_add_fd(struct event_loop *el, int fd, short events, event_loop_fd_handler handler, void *ctx);
void event_loop_remove_fd(struct event_loop *el, int fd);
void event_loop_free(struct event_loop *el);

// returns # events processed, or negative return of poll or a handler
int event_loop_poll(struct event_loop *el);

