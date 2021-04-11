//
// Created by lake on 2020/3/13.
//
#ifndef FFMPEG421DEMO_L_PACKETQUEUE_H
#define FFMPEG421DEMO_L_PACKETQUEUE_H

#include <pthread.h>
#include <libavcodec/avcodec.h>

//包列表
typedef struct MyAVPacketList {
    AVPacket pkt;
    struct LAVPacketList *next;
    int serial;
} MyAVPacketList;

//包队列
typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int queue_attachments_req;
    int serial;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} PacketQueue;

int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);

int packet_queue_init(PacketQueue *q);

int packet_queue_put(PacketQueue *q, AVPacket *pkt);

int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);

void packet_queue_flush(PacketQueue *q);

void packet_queue_destroy(PacketQueue *q);

void packet_queue_abort(PacketQueue *q);

void packet_queue_start(PacketQueue *q);

#endif //FFMPEG421DEMO_L_PACKETQUEUE_H
