//
// Created by lake on 2020/3/25.
//

#ifndef FFMPEG421DEMO_L_PLAY_H
#define FFMPEG421DEMO_L_PLAY_H

#ifdef __cplusplus
extern "C" {
#endif
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "L_Clock.h"
#include "L_Decoder.h"
#ifdef __cplusplus
}
#endif

#include <android/native_window.h>

/**
 * 空包
 */
AVPacket flush_pkt;//全局变量

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

typedef struct VideoState {
    pthread_t *read_tid;
    pthread_t *play_tid;
    AVInputFormat *iformat;
    AVFormatContext *ic;
    int abort_request;//中断请求
    int paused;//暂停
    int last_paused;//上一次暂停
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    int realtime;
    int queue_attachments_req;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;//视频
    FrameQueue sampq;//音频

    Decoder auddec;
    Decoder viddec;

    int audio_stream;
    AVStream *audio_st;
    PacketQueue audioq;
    double audio_clock;
    int audio_clock_serial;
    struct AudioParams audio_tgt;
    SwrContext *swrContext;//音频格式转换 转换为pcm数据

    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    struct SwsContext *img_convert_ctx;
    int eof;

    const char *filename;
    int last_video_stream;
    int last_audio_stream;

    pthread_cond_t *continue_read_thread;
    int xpos;//当前播放位置
    double frame_timer;//显示帧的时间
    double max_frame_duration;//每帧显示最长时间

    int force_refresh;//是否更新视频帧
    int frame_drops_late;//丢帧计数
    int frame_drops_early;//丢帧计数

    int step;//暂停
} VideoState;

#endif //FFMPEG421DEMO_L_PLAY_H
