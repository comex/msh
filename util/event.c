#include "event.h"
#include "vec.h"
#include <stdio.h>

struct handler_and_ctx {
    event_loop_fd_handler handler;
    void *ctx;
};

DECL_VEC(struct pollfd, pollfd);
DECL_VEC(struct handler_and_ctx, handler_and_ctx);

struct event_loop {
    VEC_STORAGE(handler_and_ctx) hacs;
    VEC_STORAGE(pollfd) pollfds;
};

struct event_loop *event_loop_new() {
    struct event_loop *el = malloc(sizeof(*el));
    vec_storage_init(&el->hacs);
    vec_storage_init(&el->pollfds);
    return el;
}

void event_loop_add_fd(struct event_loop *el, int fd, short events, event_loop_fd_handler handler, void *ctx) {
    vappend(&el->hacs, (struct handler_and_ctx) {handler, ctx});
    vappend(&el->pollfds, (struct pollfd) {.fd = fd, .events = events});
}

void *event_loop_remove_fd(struct event_loop *el, int fd) {
    vec_foreach(&el->pollfds, i, struct pollfd *pollfd) {
        if (pollfd->fd == fd) {
            void *ret = vgetp(&el->hacs, i)->ctx;
            vec_remove(&el->pollfds, i, 1);
            vec_remove(&el->hacs, i, 1);
            return ret;
        }
    }
    fprintf(stderr, "%s: fd %d not found\n", __func__, fd);
    ensure(0);
}

void event_loop_free(struct event_loop *el) {
    vec_free(&el->hacs);
    vec_free(&el->pollfds);
}

int event_loop_poll(struct event_loop *el) {
    int ret = poll(vgetp(&el->pollfds, 0), vlen(&el->pollfds), 0);
    if (ret < 0)
        return ret;
    int n = 0;
    vec_foreach(&el->pollfds, i, struct pollfd *pollfd) {
        if (pollfd->revents) {
            struct handler_and_ctx *hac = vgetp(&el->hacs, i);
            ret = hac->handler(pollfd->fd, pollfd->revents, hac->ctx);
            if (ret < 0)
                return ret;
            n++;
        }
    }
    return n;
}
