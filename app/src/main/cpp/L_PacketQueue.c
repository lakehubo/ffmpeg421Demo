//
// Created by lake on 2020/3/13.
//
#include "L_PacketQueue.h"
#include "L_Mutex.h"
#include "L_Cond.h"
#include "L_play.h"

int packet_queue_put_private(PacketQueue *q, AVPacket *pkt) {
    MyAVPacketList *pkt1;

    if (q->abort_request)
        return -1;
    pkt1 = (MyAVPacketList *) av_malloc(sizeof(MyAVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if (pkt == &flush_pkt)
        q->serial++;
    pkt1->serial = q->serial;

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
    q->duration += pkt1->pkt.duration;
    /* XXX: should duplicate packet data in DV case */
    condSignal(q->cond);
    return 0;
}

/* packet queue handling */
int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = createMutex();
    if (!q->mutex) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", "");
        return AVERROR(ENOMEM);
    }
    q->cond = createCond();
    if (!q->cond) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", "");
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    int ret;
    lockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt);
    unlockMutex(q->mutex);
    if (pkt != &flush_pkt && ret < 0)
        av_packet_unref(pkt);
    return ret;
}

int packet_queue_put_nullpacket(PacketQueue *q, int stream_index) {
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial) {
    MyAVPacketList *pkt1;
    int ret;

    lockMutex(q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            q->duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            if (serial)
                *serial = pkt1->serial;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            condWait(q->cond, q->mutex);
        }
    }
    unlockMutex(q->mutex);
    return ret;
}

void packet_queue_flush(PacketQueue *q) {
    MyAVPacketList *pkt, *pkt1;

    lockMutex(q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    unlockMutex(q->mutex);
}

void packet_queue_destroy(PacketQueue *q) {
    packet_queue_flush(q);
    destroyMutex(q->mutex);
    destroyCond(q->cond);
}

/**
 * 队列中断
 * @param q
 */
void packet_queue_abort(PacketQueue *q) {
    lockMutex(q->mutex);
    q->abort_request = 1;
    condSignal(q->cond);
    unlockMutex(q->mutex);
}


void packet_queue_start(PacketQueue *q) {
    lockMutex(q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, &flush_pkt);
    unlockMutex(q->mutex);
}