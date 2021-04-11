//
// Created by lake on 2020/3/24.
//
#ifndef FFMPEG421DEMO_L_AUDIO_H
#define FFMPEG421DEMO_L_AUDIO_H

#include <assert.h>
#include <pthread.h>
#include <malloc.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "L_play.h"

typedef struct SLAudio {
    // engine interfaces
    SLObjectItf engineObject;
    SLEngineItf engineEngine;

    // output mix interfaces
    SLObjectItf outputMixObject;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;

    // buffer queue player interfaces
    SLObjectItf bqPlayerObject;

    SLEffectSendItf bqPlayerEffectSend;
    SLMuteSoloItf bqPlayerMuteSolo;
    SLVolumeItf bqPlayerVolume;
    SLmilliHertz bqPlayerSampleRate;

    SLAndroidSimpleBufferQueueItf audioPlayerBufferQueue;
    slAndroidSimpleBufferQueueCallback callback;
    SLPlaybackRateItf bqPlaybackRate;//播放速率
    SLPlayItf bqPlayerPlay;

    void *userdata;//用户自定义数据指针
} SLAudio;

void createEngine(SLAudio *ad);

void createBufferQueueAudioPlayer(SLAudio *ad, int sampleRate, int channel);

void shutdown(SLAudio *ad);

#endif //FFMPEG421DEMO_L_AUDIO_H
