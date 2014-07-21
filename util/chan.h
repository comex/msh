#pragma once
// thin wrapper around pipe()

#include "event.h"

struct chanw {
   int wfd;
   struct chanr *chanr;
};

struct chanr {
   int wfd, rfd;
};

// must free data if desired
typedef int (*event_loop_chan_handler)(struct chanr *cr, void *data, void *ctx);

void chanr_init(struct chanr *cr);
struct chanw chanr_make_writer(struct chanr *cr);

void event_loop_add_chanr(struct event_loop *el, struct chanr *cr, event_loop_chan_handler handler, void *ctx);
void *event_loop_remove_chanr(struct event_loop *el, struct chanr *cr);

void chanr_del(struct chanr *chanr);

// I hope you don't run out of buffer!
void chanw_send(struct chanw *chanw, void *data);
void chanw_del(struct chanw *chanw);
