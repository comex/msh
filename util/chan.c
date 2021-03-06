#include "chan.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>

void chanr_init(struct chanr *cr) {
   int fds[2];
   ensure(!pipe(fds));
   cr->wfd = fds[1];
   cr->rfd = fds[0];
}

struct chanw chanr_make_writer(struct chanr *cr) {
   int dupped = dup(cr->wfd);
   ensure(dupped != -1);
   return (struct chanw) {dupped, cr};
}

struct chan_ctx {
   struct chanr *chanr;
   event_loop_chan_handler userhandler;
   void *userctx;
};

int chanr_fd_handler(int fd, short revents, void *ctx) {
   struct chan_ctx *cctx = ctx;
   ensure(revents & POLLIN);
   void *data;
   ensure(read(fd, &data, sizeof(data)) == (ssize_t) sizeof(data));
   return cctx->userhandler(cctx->chanr, data, cctx->userctx);
}

void event_loop_add_chanr(struct event_loop *el, struct chanr *cr, event_loop_chan_handler handler, void *ctx) {
   struct chan_ctx *cctx = malloc(sizeof(*cctx));
   cctx->chanr = cr;
   cctx->userhandler = handler;
   cctx->userctx = ctx;

   event_loop_add_fd(el, cr->rfd, POLLIN, chanr_fd_handler, cctx);
}

void *event_loop_remove_chanr(struct event_loop *el, struct chanr *cr) {
   struct chan_ctx *cctx = event_loop_remove_fd(el, cr->rfd);
   void *userctx = cctx->userctx;
   free(cctx);
   return userctx;
}

void chanr_del(struct chanr *chanr) {
   close(chanr->wfd);
   close(chanr->rfd);
}

void chanw_send(struct chanw *chanw, void *data) {
   static_assert(sizeof(data) <= PIPE_BUF, "PIPE_BUF is actually POSIX guaranteed >= 512 but whatever");
   ensure(write(chanw->wfd, &data, sizeof(data)) == (ssize_t) sizeof(data));
}

void chanw_del(struct chanw *chanw) {
   close(chanw->wfd);
}
