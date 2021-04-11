//
// Created by lake on 2020/3/13.
//
#include "L_Decoder.h"
#include "L_Thread.h"
#include "L_Cond.h"
#include "L_FrameQueue.h"
#include "L_play.h"
#include <LogUtil.h>

static int decoder_reorder_pts = -1;

void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue,
                  pthread_cond_t *empty_queue_cond) {
    LOGCATI("decoder_init");
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
    d->pkt_serial = -1;
}

/**
 * packet 解析成 frame
 * @param d
 * @param frame
 * @return
 */
int decoder_decode_frame(Decoder *d, AVFrame *frame) {
    LOGCATI("decoder_decode_frame");
    int ret = AVERROR(EAGAIN);
    for (;;) {
        AVPacket pkt;
        if (d->queue->serial == d->pkt_serial) {
            do {
                if (d->queue->abort_request)
                    return -1;
                switch (d->avctx->codec_type) {
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            AVRational tb = (AVRational) {1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) {
                            if (decoder_reorder_pts == -1) {
                                frame->pts = frame->best_effort_timestamp;
                            } else if (!decoder_reorder_pts) {
                                frame->pts = frame->pkt_dts;
                            }
                        }
                        break;
                }
                if (ret == AVERROR_EOF) {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0)
                    return 1;
            } while (ret != AVERROR(EAGAIN));
        }

        do {
            if (d->queue->nb_packets == 0)
                condSignal(d->empty_queue_cond);
            if (d->packet_pending) {
                av_packet_move_ref(&pkt, &d->pkt);
                d->packet_pending = 0;
            } else {
                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
                    return -1;
                LOGCATI("GET PKT");
            }
        } while (d->queue->serial != d->pkt_serial);

        if (pkt.data == flush_pkt.data) {
            avcodec_flush_buffers(d->avctx);
            d->finished = 0;
            d->next_pts = d->start_pts;
            d->next_pts_tb = d->start_pts_tb;
        } else {
            if (d->avctx->codec_type == AVMEDIA_TYPE_VIDEO ||
                d->avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
                LOGCATI("SEND PKT!!");
                if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
                    av_log(d->avctx, AV_LOG_ERROR,
                           "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                    d->packet_pending = 1;
                    av_packet_move_ref(&d->pkt, &pkt);
                }
            }
            av_packet_unref(&pkt);
        }
    }
}

int decoder_start(Decoder *d, void *(*fn)(void *), void *arg) {
    LOGCATI("decoder_start");
    packet_queue_start(d->queue);
    d->decoder_tid = createThread(fn, arg);
    if (!d->decoder_tid) {
        LOGCATE("createThread(): %s\n", "");
        return AVERROR(ENOMEM);
    }
    return 0;
}

void decoder_abort(Decoder *d, FrameQueue *fq) {
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    waitThread(*d->decoder_tid);
    d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}

void decoder_destroy(Decoder *d) {
    av_packet_unref(&d->pkt);
    avcodec_free_context(&d->avctx);
}