//
// Created by lake on 2020/3/13.
//

#include <LogUtil.h>
#include "L_FrameQueue.h"
#include "L_Mutex.h"
#include "L_Cond.h"

void frame_queue_unref_item(Frame *vp) {
    av_frame_unref(vp->frame);
}

int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last) {
    int i;
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = createMutex())) {
        LOGCATE("createMutex():FrameQueue error");
        return AVERROR(ENOMEM);
    }
    if (!(f->cond = createCond())) {
        LOGCATE("createCond():FrameQueue error");
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

void frame_queue_destory(FrameQueue *f) {
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
    }
    destroyMutex(f->mutex);
    destroyCond(f->cond);
}

void frame_queue_signal(FrameQueue *f) {
    lockMutex(f->mutex);
    condSignal(f->cond);
    unlockMutex(f->mutex);
}

Frame *frame_queue_peek(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

Frame *frame_queue_peek_next(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

Frame *frame_queue_peek_last(FrameQueue *f) {
    return &f->queue[f->rindex];
}

Frame *frame_queue_peek_writable(FrameQueue *f) {
/* wait until we have space to put a new frame */
    lockMutex(f->mutex);
    while (f->size >= f->max_size &&
           !f->pktq->abort_request) {
        condWait(f->cond, f->mutex);
    }
    unlockMutex(f->mutex);

    if (f->pktq->abort_request)
        return NULL;

    return &f->queue[f->windex];
}

Frame *frame_queue_peek_readable(FrameQueue *f) {
    lockMutex(f->mutex);
    while (f->size - f->rindex_shown <= 0 &&
           !f->pktq->abort_request) {
        condWait(f->cond, f->mutex);
    }
    unlockMutex(f->mutex);

    if (f->pktq->abort_request)
        return NULL;

    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void frame_queue_push(FrameQueue *f) {
    if (++f->windex == f->max_size)
        f->windex = 0;
    lockMutex(f->mutex);
    f->size++;
    condSignal(f->cond);
    unlockMutex(f->mutex);
}

void frame_queue_next(FrameQueue *f) {
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
        f->rindex = 0;
    lockMutex(f->mutex);
    f->size--;
    condSignal(f->cond);
    unlockMutex(f->mutex);
}

int frame_queue_nb_remaining(FrameQueue *f) {
    return f->size - f->rindex_shown;
}

int64_t frame_queue_last_pos(FrameQueue *f) {
    Frame *fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->pktq->serial)
        return fp->pos;
    else
        return -1;
}