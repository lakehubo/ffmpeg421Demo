#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <LogUtil.h>

extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libswscale/swscale.h"
#include "include/libavcodec/jni.h"
#include "include/libavutil/frame.h"
#include "include/libavutil/imgutils.h"
#include "include/libavformat/avformat.h"
#include "L_Audio.h"
#include "L_Mutex.h"
#include "L_Cond.h"
#include "L_Thread.h"
#include "L_PacketQueue.h"
#include "L_Decoder.h"
#include "L_Clock.h"
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include "L_FrameQueue.h"
#include <pthread.h>
#include "L_Callback.h"
}

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <LGLRenderContext.h>
#include <GLRenderCall.h>
#include "L_play.h"


#define CLASS   com_lake_ffmpeg421demo_LPlayView
//宏NAME(FUNC)的定义，一定按照下面的方法定义，才能合成出正确的函数名称
#define JNI_METHOD3(CLASS3, FUNC3) Java_##CLASS3##_##FUNC3
#define JNI_METHOD2(CLASS2, FUNC2) JNI_METHOD3(CLASS2, FUNC2)
#define JNI_METHOD(FUNC) JNI_METHOD2(CLASS, FUNC)

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define REFRESH_RATE 0.01
#define CALLBAKC_PID 6

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

static JavaVM *javaVm;
static uint8_t *nextBuffer;
static unsigned int nextSize = 0;

static VideoState *cur_stream;
static SLAudio sla;//opensl播放器
static int64_t audio_callback_time;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;//同步模式 现在只支持音频同步视频
static int framedrop = -1;//是否需要丢帧
static int loop = 1;//是否循环
static int seek_by_bytes = -1;//自动 0=off 1=on -1=auto
static float seek_interval = 10;
static int64_t start_time = AV_NOPTS_VALUE;
static int64_t duration = AV_NOPTS_VALUE;
static int autoexit;
static CallbackContext callbackContext;
static GLRenderCall *glRenderCall;

//是否是实时视频
static int is_realtime(AVFormatContext *s) {
    if (!strcmp(s->iformat->name, "rtp")
        || !strcmp(s->iformat->name, "rtsp")
        || !strcmp(s->iformat->name, "sdp")
            )
        return 1;

    if (s->pb && (!strncmp(s->url, "rtp:", 4)
                  || !strncmp(s->url, "udp:", 4)
    )
            )
        return 1;
    return 0;
}

/* seek in the stream */
static void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes) {
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        condSignal(is->continue_read_thread);
    }
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState *is) {
    if (is->paused) {
        is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }
    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

static void toggle_pause(VideoState *is) {
    stream_toggle_pause(is);
    is->step = 0;
    if (!is->paused && &sla && &sla.callback) {
        sla.callback(sla.audioPlayerBufferQueue, sla.userdata);
    }
}

static void step_to_next_frame(VideoState *is) {
    /* if the stream is paused unpause it, then step */
    if (is->paused)
        stream_toggle_pause(is);
    is->step = 1;
}

static int get_master_sync_type(VideoState *is) {
    if (av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

/* get the current master clock value */
static double get_master_clock(VideoState *is) {
    double val;
    switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            val = get_clock(&is->vidclk);
            break;
        case AV_SYNC_AUDIO_MASTER:
            val = get_clock(&is->audclk);
            break;
        default:
            val = get_clock(&is->extclk);
            break;
    }
    return val;
}

static int get_video_frame(VideoState *is, AVFrame *frame) {
    LOGCATI("get_video_frame");
    int ret;
    if ((ret = decoder_decode_frame(&is->viddec, frame)) < 0)
        return -1;
    if (ret) {
        double dpts = NAN;
        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(is->video_st->time_base) * frame->pts;
        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
        if (framedrop > 0 ||
            (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {//是否需要丢帧
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - get_master_clock(is);
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff < 0 &&
                    is->viddec.pkt_serial == is->vidclk.serial &&
                    is->videoq.nb_packets) {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    ret = 0;
                }
            }
        }
    }
    return ret;
}

static int
queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos,
              int serial) {
    Frame *vp;
    if (!(vp = frame_queue_peek_writable(&is->pictq)))
        return -1;
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->pictq);
    return 0;
}

static int audio_decode_frame(VideoState *is) {
    Frame *af;
    int len;
    int sampled_data_size;

    if (is->paused)
        return -1;

    do {
        if (!(af = frame_queue_peek_readable(&is->sampq))) {
            return -1;
        }
        frame_queue_next(&is->sampq);
    } while (af->serial != is->audioq.serial);

    if (af->frame->extended_data) {//格式转换 将音频数据转换成 PCM
        const uint8_t **in = (const uint8_t **) af->frame->extended_data;
        uint8_t **out = &nextBuffer;
        int out_count = af->frame->sample_rate;
        int out_size = av_samples_get_buffer_size(NULL, af->frame->channels, out_count,
                                                  AV_SAMPLE_FMT_S16, 0);
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }

        av_fast_malloc(&nextBuffer, &nextSize, (size_t) out_size);
        if (!nextBuffer)
            return AVERROR(ENOMEM);

        len = swr_convert(is->swrContext, out, out_count, in, af->frame->nb_samples);

        if (len < 0) {
            return -1;
        }

        sampled_data_size = len * af->frame->channels *
                            av_get_bytes_per_sample((AVSampleFormat) AV_SAMPLE_FMT_S16);

        //音频时钟更新
        if (!isnan(af->pts))
            is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
        else
            is->audio_clock = NAN;
        is->audio_clock_serial = af->serial;

        return sampled_data_size;
    }
    return -1;
}

/**
 * openSL 音频播放缓存队列 每播放完一帧会自动回调该方法 所以在每帧播放完成后再解下一帧
 */
static void audioPlayer(SLAndroidSimpleBufferQueueItf bq, void *context) {
    assert(sla.audioPlayerBufferQueue == bq);
    VideoState *is = (VideoState *) context;
    int audio_size = 0;
    audio_callback_time = av_gettime_relative();

    audio_size = audio_decode_frame(is);

    if (NULL != nextBuffer && audio_size > 0) {
        SLresult result;
        // enqueue another buffer
        result = (*sla.audioPlayerBufferQueue)->Enqueue(sla.audioPlayerBufferQueue, nextBuffer,
                                                        audio_size);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        (void) result;
    }
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(is->audio_clock)) {//存储音频时钟 便于同步视频
        set_clock_at(&is->audclk,
                     is->audio_clock -
                     (double) (audio_size) / is->audio_tgt.bytes_per_sec,
                     is->audio_clock_serial, audio_callback_time / 1000000.0);
        sync_clock_to_slave(&is->extclk, &is->audclk);
        if (&callbackContext) {
            attachCallback(&callbackContext);
            double cur = get_clock(&cur_stream->audclk);
            double tns = cur_stream->ic->duration / 1000000LL;
            callbackProgress(&callbackContext, cur / tns);
        }
    }
}

static void *audio_thread(void *arg) {
    LOGCATI("audio_thread");
    VideoState *is = (VideoState *) arg;
    AVFrame *frame = av_frame_alloc();
    Frame *af;
    int got_frame = 0;
    AVRational tb;
    int ret = 0;
    bool need_openSl = true;//是否需要开启音频

    if (!frame)
        return NULL;

    do {
        if ((got_frame = decoder_decode_frame(&is->auddec, frame)) < 0)
            goto the_end;
        if (got_frame) {
            tb = (AVRational) {1, frame->sample_rate};

            if (!(af = frame_queue_peek_writable(&is->sampq)))
                goto the_end;

            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            af->pos = frame->pkt_pos;
            af->serial = is->auddec.pkt_serial;
            af->duration = av_q2d((AVRational) {frame->nb_samples, frame->sample_rate});

            av_frame_move_ref(af->frame, frame);
            frame_queue_push(&is->sampq);
            if (&sla && &sla.callback && need_openSl) {//当有音频数据后再触发音频播放
                need_openSl = false;
                sla.callback(sla.audioPlayerBufferQueue, sla.userdata);
            }
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
    the_end:
    detachCallback(&callbackContext);
    shutdown(&sla);
    av_frame_free(&frame);
    return NULL;
}

//获取当前视频播放延迟
static double compute_target_delay(double delay, VideoState *is) {
    double sync_threshold, diff = 0;
    /* if video is slave, we try to correct big delays by
       duplicating or deleting a frame */
    diff = get_clock(&is->vidclk) - get_master_clock(is);
    /* skip or repeat frame. We take into account the
       delay to compute the threshold. I still don't know
       if it is the best guess */
    sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
        if (diff <= -sync_threshold)
            delay = FFMAX(0, delay + diff);
        else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
            delay = delay + diff;
        else if (diff >= sync_threshold)
            delay = 2 * delay;
    }
    LOGCATI("video: delay=%0.3f A-V=%f\n",
            delay, -diff);
    return delay;
}

//判断该帧显示时间
static double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration) {
            return vp->duration;
        } else {
            return duration;
        }
    } else {
        return 0.0;
    }
}

static void decode_video_frame(VideoState *is, double *remaining_time) {
    double time;
    if (is->video_st) {
        retry:
        if (frame_queue_nb_remaining(&is->pictq) == 0) {//队列为空

        } else {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;

            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->pictq);
            vp = frame_queue_peek(&is->pictq);
            if (vp->serial != is->videoq.serial) {
                frame_queue_next(&is->pictq);
                goto retry;
            }
            if (lastvp->serial != vp->serial) {
                is->frame_timer = av_gettime_relative() / 1000000.0;
            }

            if (is->paused)
                goto display;

            /* compute nominal last_duration */
            last_duration = vp_duration(is, lastvp, vp);
            delay = compute_target_delay(last_duration, is);

            time = av_gettime_relative() / 1000000.0;
            if (time < is->frame_timer + delay) {
                *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
                goto display;
            }
            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->frame_timer = time;

            lockMutex(is->pictq.mutex);
            if (!isnan(vp->pts)) {
                set_clock(&is->vidclk, vp->pts, vp->serial);
                sync_clock_to_slave(&is->extclk, &is->vidclk);
            }
            unlockMutex(is->pictq.mutex);

            if (frame_queue_nb_remaining(&is->pictq) > 1) {//丢帧处理
                Frame *nextvp = frame_queue_peek_next(&is->pictq);
                duration = vp_duration(is, vp, nextvp);
                if (!is->step && (framedrop > 0 || (framedrop && get_master_sync_type(is) !=
                                                                 AV_SYNC_VIDEO_MASTER)) &&
                    time > is->frame_timer + duration) {
                    is->frame_drops_late++;
                    frame_queue_next(&is->pictq);
                    goto retry;
                }
            }
            frame_queue_next(&is->pictq);
            is->force_refresh = 1;

            if (is->step && !is->paused)
                stream_toggle_pause(is);
        }
        display:
        if (is->force_refresh && is->pictq.rindex_shown) {
            Frame *vp;
            vp = frame_queue_peek_last(&is->pictq);
            LGLRenderContext::GetInstance()->SetFrameData(vp->frame);
            glRenderCall->callRequestRenderInThr();//手动出发requestRender();
        }
        is->force_refresh = 0;
    }
}

static void *videoPlayer(void *video_state) {
    VideoState *is = (VideoState *) video_state;
    double remaining_time = 0.0;
    //render attach
    glRenderCall->attachCall();
    for (;;) {
        if (is->abort_request)
            break;
        if (remaining_time > 0.0)
            av_usleep(static_cast<unsigned>(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (!is->paused || is->force_refresh)
            decode_video_frame(is, &remaining_time);
    }
    //render call end
    glRenderCall->detachCall();
    return NULL;
}

static void *video_thread(void *arg) {
    LOGCATI("video_thread");
    VideoState *is = (VideoState *) arg;
    AVFrame *frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);
    bool first = true;
    if (!frame)
        return NULL;
    for (;;) {
        ret = get_video_frame(is, frame);
        if (ret < 0)
            goto the_end;
        if (!ret)
            continue;
        //根据帧像素格式初始化opengl的绘制program
        if (first && frame->format != AV_PIX_FMT_NONE) {
            first = false;
            LGLRenderContext::GetInstance()->initProgram(frame->format);
        }

        duration = (frame_rate.num && frame_rate.den ? av_q2d(
                (AVRational) {frame_rate.den, frame_rate.num}) : 0);
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        ret = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
        av_frame_unref(frame);
        if (ret < 0)
            goto the_end;
    }
    the_end:
    av_frame_free(&frame);
    return NULL;
};


/**
 * 初始化音频播放器
 */
static int audio_open(void *opaque, AVCodecContext *avCodecContext) {
    VideoState *is = (VideoState *) opaque;
    int sampleRate = avCodecContext->sample_rate;
    int channels = avCodecContext->channels;

    int64_t dec_channel_layout;
    is->swrContext = swr_alloc();
    if (!is->swrContext) {
        return -1;
    }
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;

    dec_channel_layout = (avCodecContext->channel_layout && avCodecContext->channels ==
                                                            av_get_channel_layout_nb_channels(
                                                                    avCodecContext->channel_layout))
                         ?
                         avCodecContext->channel_layout :
                         av_get_default_channel_layout(avCodecContext->channels);

    is->swrContext = swr_alloc_set_opts(NULL, avCodecContext->channel_layout, out_format,
                                        sampleRate,
                                        dec_channel_layout, avCodecContext->sample_fmt,
                                        sampleRate, 0,
                                        NULL);

    if (!is->swrContext || swr_init(is->swrContext) < 0) {
        swr_free(&is->swrContext);
        return -1;
    }

    is->audio_tgt.fmt = out_format;
    is->audio_tgt.freq = sampleRate;
    is->audio_tgt.channel_layout = dec_channel_layout;
    is->audio_tgt.channels = channels;
    is->audio_tgt.frame_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, 1,
                                                          is->audio_tgt.fmt, 1);
    is->audio_tgt.bytes_per_sec = av_samples_get_buffer_size(NULL, is->audio_tgt.channels,
                                                             is->audio_tgt.freq,
                                                             is->audio_tgt.fmt,
                                                             1);
    if (is->audio_tgt.bytes_per_sec <= 0 || is->audio_tgt.frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }

    nextSize = 8192;
    nextBuffer = (uint8_t *) malloc((size_t) nextSize);

    //初始化openSL引擎
    createEngine(&sla);
    sla.userdata = is;
    sla.callback = audioPlayer;
    createBufferQueueAudioPlayer(&sla, sampleRate, channels);
    return 0;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState *is, int stream_index) {
    LOGCATI("stream_component_open");
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    int ret = 0;
    long cpu_count = 4;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return AVERROR(ENOMEM);
    cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    LOGCATI("CPU 核数=%d", (int) cpu_count);
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
        goto fail;
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;

    if (avctx->codec_id == AV_CODEC_ID_H264)
        codec = avcodec_find_decoder_by_name("h264_mediacodec");
    else
        codec = avcodec_find_decoder(avctx->codec_id);

    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO)
        avctx->thread_count = (int) cpu_count > 1 ? (int) cpu_count - 1 : 1;//视频解码线程指定
    is->last_video_stream = stream_index;
    if (avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        is->last_audio_stream = stream_index;
    if (!codec) {
        ret = AVERROR(EINVAL);
        goto fail;
    }
    avctx->codec_id = codec->id;
    avctx->lowres = 0;

    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
        goto fail;
    }

    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        if ((ret = audio_open(is, avctx)) < 0)
            goto fail;  //音频播放参数设置 以及开启音频播放器

        is->audio_stream = stream_index;
        is->audio_st = ic->streams[stream_index];

        decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
        if ((is->ic->iformat->flags &
             (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
            !is->ic->iformat->read_seek) {
            is->auddec.start_pts = is->audio_st->start_time;
            is->auddec.start_pts_tb = is->audio_st->time_base;
        }
        if ((ret = decoder_start(&is->auddec, audio_thread, is)) < 0)
            goto out;
    } else if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        is->video_stream = stream_index;
        is->video_st = ic->streams[stream_index];

        decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
        if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
            goto out;
        is->queue_attachments_req = 1;
    }

    goto out;

    fail:
    avcodec_free_context(&avctx);
    out:
    LOGCATE("decoder_start end");
    return ret;
}

static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return stream_id < 0 ||
           queue->abort_request ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES &&
           (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

static void *read_thread(void *arg) {
    VideoState *is = (VideoState *) arg;
    AVFormatContext *ic = NULL;
    int err, i, ret;
    AVPacket pkt1, *pkt = &pkt1;
    int video_index = -1;
    int audio_index = -1;
    int64_t stream_start_time;
    pthread_mutex_t *wait_mutex;
    wait_mutex = createMutex();
    int64_t pkt_ts;

    if (!&wait_mutex) {
        LOGCATE("CreateMutex(): %s\n", "wait_mutex is null");
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    is->eof = 0;

    ic = avformat_alloc_context();
    if (!ic) {
        LOGCATE("Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    err = avformat_open_input(&ic, is->filename, NULL, NULL);
    if (err < 0) {
        LOGCATE("Could not open the video file", is->filename, err);
        ret = -1;
        goto fail;
    }
    is->ic = ic;
    err = avformat_find_stream_info(ic, NULL);
    if (err < 0) {
        LOGCATE("%s: could not find codec parameters\n", is->filename);
        ret = -1;
        goto fail;
    }
    if (seek_by_bytes < 0)
        seek_by_bytes =
                !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;//每帧最长显示时间

    /* if seeking requested, we execute it */
    if (start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = start_time;
        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                   is->filename, (double) timestamp / AV_TIME_BASE);
        }
    }

    is->realtime = is_realtime(ic);

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        if (AVMEDIA_TYPE_VIDEO == st->codecpar->codec_type) {
            video_index = i;
        }
        if (AVMEDIA_TYPE_AUDIO == st->codecpar->codec_type) {
            audio_index = i;
        }
    }
    video_index = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, video_index, -1, NULL, 0);
    if (video_index >= 0) {
        stream_component_open(is, video_index);
    }
    audio_index = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, audio_index, -1, NULL, 0);
    if (audio_index >= 0) {
        stream_component_open(is, audio_index);
    }
    if (video_index < 0 && audio_index < 0) {
        LOGCATE("failed to open file!");
        goto fail;
    }
    for (;;) {
        if (is->abort_request)
            break;
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused)
                is->read_pause_return = av_read_pause(ic);
            else
                av_read_play(ic);
        }
        //跳转
        if (is->seek_req) {
            int64_t seek_target = is->seek_pos;
            int64_t seek_min = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
            int64_t seek_max = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
//FIXME the +-2 is due to rounding being not done in the correct direction in generation
//of the seek_pos/seek_rel variables
            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max,
                                     is->seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n", is->ic->url);
            } else {
                if (is->audio_stream >= 0) {
                    packet_queue_flush(&is->audioq);
                    packet_queue_put(&is->audioq, &flush_pkt);
                }
                if (is->video_stream >= 0) {
                    packet_queue_flush(&is->videoq);
                    packet_queue_put(&is->videoq, &flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                    set_clock(&is->extclk, NAN, 0);
                } else {
                    set_clock(&is->extclk, seek_target / (double) AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused) {
                step_to_next_frame(is);
                if (&callbackContext) {
                    attachCallback(&callbackContext);
                    double pos = (double) is->seek_pos / (double) is->ic->duration;
                    callbackProgress(&callbackContext, pos);
                    detachCallback(&callbackContext);
                }
            }
        }
        if (is->queue_attachments_req) {
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket copy = {0};
                if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
                    goto fail;
                packet_queue_put(&is->videoq, &copy);
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }

        /* if the queue are full, no need to read more */
        if (is->videoq.size > MAX_QUEUE_SIZE ||
            (stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
             stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq))) {
            lockMutex(wait_mutex);
            waitTimeout(is->continue_read_thread, wait_mutex, 10);
            unlockMutex(wait_mutex);
            continue;
        }
        //播放完后的操作
        if (!is->paused &&
            (!is->audio_st || (is->auddec.finished == is->audioq.serial &&
                               frame_queue_nb_remaining(&is->sampq) == 0)) &&
            (!is->video_st || (is->viddec.finished == is->videoq.serial &&
                               frame_queue_nb_remaining(&is->pictq) == 0))) {
            if (loop != 1 && (!loop || --loop)) {
                stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
            } else if (autoexit) {
                ret = AVERROR_EOF;
                goto fail;
            }
        }
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0)
                    packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (is->audio_stream >= 0)
                    packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                is->eof = 1;
            }
            if (ic->pb && ic->pb->error)
                break;
            lockMutex(wait_mutex);
            waitTimeout(is->continue_read_thread, wait_mutex, 10);
            unlockMutex(wait_mutex);
            continue;
        } else {
            is->eof = 0;
        }
        if (pkt->stream_index == is->audio_stream) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream) {
            packet_queue_put(&is->videoq, pkt);
        } else {
            av_packet_unref(pkt);
        }
    }

    fail:
    if (ic && !is->ic)
        avformat_close_input(&ic);

    destroyMutex(wait_mutex);
    return NULL;
}

static void stream_component_close(VideoState *is, int stream_index) {
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;
    if (stream_index < 0)
        return;
    codecpar = ic->streams[stream_index]->codecpar;

    if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        decoder_abort(&is->viddec, &is->pictq);
        decoder_destroy(&is->viddec);
    } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        decoder_abort(&is->auddec, &is->sampq);
        decoder_destroy(&is->auddec);
        swr_free(&is->swrContext);
    }
    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            is->audio_st = NULL;
            is->audio_stream = -1;
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_st = NULL;
            is->video_stream = -1;
            break;
        default:
            break;
    }
}

static void stream_close(VideoState *is) {
    is->abort_request = 1;
    waitThread(*is->read_tid);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);
    if (is->audio_stream >= 0) {
        stream_component_close(is, is->audio_stream);
    }
    avformat_close_input(&is->ic);

    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);

    frame_queue_destory(&is->pictq);
    frame_queue_destory(&is->sampq);

    destroyCond(is->continue_read_thread);
    sws_freeContext(is->img_convert_ctx);
    swr_free(&is->swrContext);
    av_free(is);
}

static VideoState *stream_open(const char *filename) {
    VideoState *is;
    is = (VideoState *) av_mallocz(sizeof(VideoState));
    if (!is)
        return NULL;
    is->filename = av_strdup(filename);
    if (!is->filename)
        goto fail;

    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
        goto fail;
    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
        goto fail;

    if (packet_queue_init(&is->videoq) < 0 ||
        packet_queue_init(&is->audioq) < 0) {
        goto fail;
    }

    init_clock(&is->vidclk, &is->videoq.serial);
    init_clock(&is->audclk, &is->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);
    is->audio_clock_serial = -1;

    is->continue_read_thread = createCond();
    if (!is->continue_read_thread) {
        goto fail;
    }
    is->read_tid = createThread(read_thread, is);
    if (!is->read_tid)
        goto fail;

    is->play_tid = createThread(videoPlayer, is);
    if (!is->play_tid) {
        LOGCATE("开启解码/播放线程失败！");
        fail:
        stream_close(is);
        return NULL;
    }
    return is;
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @param env
 * @param instance
 * @param input_jstr
 * @param surface
 * @return
 */
JNIEXPORT jint JNI_METHOD(native_1play)(JNIEnv *env, jobject instance, jstring input_jstr) {
    //sd卡中的视频文件地址,可自行修改或者通过jni传入
    const char *file_name = env->GetStringUTFChars(input_jstr, NULL);
    LOGCATI("file_name:%s\n", file_name);
    avformat_network_init();
    if (!file_name) {
        LOGCATE("An input file must be specified\n");
        return -1;
    }
    glRenderCall = new GLRenderCall();
    glRenderCall->initCall(javaVm, env, instance);

    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *) &flush_pkt;

    cur_stream = stream_open(file_name);
    if (!cur_stream) {
        av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNI_METHOD(native_1togglePause)(JNIEnv *env, jobject instance) {
    if (cur_stream) {
        toggle_pause(cur_stream);
        return 0;
    }
    return -1;
}

JNIEXPORT jint JNI_METHOD(native_1doSeek)(JNIEnv *env, jobject instance, double frac) { //跳转的百分比
    if (cur_stream) {
        int64_t ts;
        int ns, hh, mm, ss;
        int tns, thh, tmm, tss;
        tns = cur_stream->ic->duration / 1000000LL;
        thh = tns / 3600;
        tmm = (tns % 3600) / 60;
        tss = (tns % 60);
        ns = frac * tns;
        hh = ns / 3600;
        mm = (ns % 3600) / 60;
        ss = (ns % 60);
        LOGCATI("Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n",
                frac * 100,
                hh, mm, ss, thh, tmm, tss);
        ts = frac * cur_stream->ic->duration;
        if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
            ts += cur_stream->ic->start_time;
        stream_seek(cur_stream, ts, 0, 0);
        return 0;
    }
    return -1;
}

JNIEXPORT jint JNI_METHOD(native_1stop)(JNIEnv *env, jobject thiz) {
    if (cur_stream)
        stream_close(cur_stream);
    if (glRenderCall)
        glRenderCall->releaseCallObj(env);
        delete (glRenderCall);
    avformat_network_deinit();
    return 0;
}

JNIEXPORT void JNI_METHOD(native_1setPlayCallback)(JNIEnv *env, jobject instance,
                                                   jobject callback) {
    jobject obj = env->NewGlobalRef(callback);
    createCallback(&callbackContext, javaVm, env, obj);
}

JNIEXPORT void JNI_METHOD(native_1renderUnInit)(JNIEnv *env, jobject instance) {
    LGLRenderContext::DestroyInstance();
}

JNIEXPORT void JNI_METHOD(native_1renderOnSurfaceCreated)(JNIEnv *env, jobject instance) {
    LGLRenderContext::GetInstance()->OnSurfaceCreated();
}

JNIEXPORT void JNI_METHOD(native_1renderOnSurfaceChanged)
        (JNIEnv *env, jobject instance, jint width, jint height) {
    LGLRenderContext::GetInstance()->OnSurfaceChanged(width, height);
}

JNIEXPORT void JNI_METHOD(native_1renderOnDrawFrame)(JNIEnv *env, jobject instance) {
    LGLRenderContext::GetInstance()->OnDrawFrame();
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)//这个类似android的生命周期，加载jni的时候会自己调用
{
    LOGCATI("ffmpeg JNI_OnLoad");
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    javaVm = vm;
    av_jni_set_java_vm(vm, reserved);
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif