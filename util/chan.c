#include "chan.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

void chanr_init(struct chanr *cr) {
   int fds[2];
   ensure(!pipe(fds));
   cr->wfd = fds[1];
   cr->rfd = fds[0];
   mtx_init(&cr->send_mtx, mtx_plain);
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
   size_t msg_size;
   ensure(read(fd, &msg_size, sizeof(msg_size)) == (ssize_t) sizeof(msg_size));
   void *rbuf = malloc(msg_size);
   ensure(read(fd, rbuf, msg_size) == (ssize_t) msg_size);
   return cctx->userhandler(cctx->chanr, rbuf, msg_size, cctx->userctx);
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
   mtx_destroy(&chanr->send_mtx);
}

void chanw_send(struct chanw *chanw, void *data, size_t size) {
   mtx_lock(&chanw->chanr->send_mtx);
   ensure(write(chanw->wfd, &size, sizeof(size)) == (ssize_t) sizeof(size));
   ensure(write(chanw->wfd, data, size) == (ssize_t) size);
   mtx_unlock(&chanw->chanr->send_mtx);
}

void chanw_del(struct chanw *chanw) {
   close(chanw->wfd);
}
