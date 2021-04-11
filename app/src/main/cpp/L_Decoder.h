//
// Created by lake on 2020/3/13.
//
#ifndef FFMPEG421DEMO_L_DECODER_H
#define FFMPEG421DEMO_L_DECODER_H
#include <libavcodec/avcodec.h>
#include "L_PacketQueue.h"
#include "L_FrameQueue.h"

typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    pthread_cond_t *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    pthread_t *decoder_tid;
} Decoder;

void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue,pthread_cond_t *empty_queue_cond);
int decoder_decode_frame(Decoder *d, AVFrame *frame);
int decoder_start(Decoder *d, void *(*fn)(void *), void *arg);
void decoder_abort(Decoder *d,FrameQueue *fq);
void decoder_destroy(Decoder *d);

#endif //FFMPEG421DEMO_L_DECODER_H
