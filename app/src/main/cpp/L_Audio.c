//
// Created by lake on 2020/3/23.
//
// for native audio
#include "L_Audio.h"

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
        SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

void createEngine(SLAudio *ad) {
    memset(ad, 0, sizeof(SLAudio));
    SLresult result;

    // create engine
    result = slCreateEngine(&ad->engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the engine
    result = (*ad->engineObject)->Realize(ad->engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the engine interface, which is needed in order to create other objects
    result = (*ad->engineObject)->GetInterface(ad->engineObject, SL_IID_ENGINE, &ad->engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*ad->engineEngine)->CreateOutputMix(ad->engineEngine, &ad->outputMixObject, 1, ids,
                                                  req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the output mix
    result = (*ad->outputMixObject)->Realize(ad->outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*ad->outputMixObject)->GetInterface(ad->outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                  &ad->outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*ad->outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                ad->outputMixEnvironmentalReverb, &reverbSettings);
        (void) result;
    }
}

void createBufferQueueAudioPlayer(SLAudio *ad, int sampleRate, int channel) {

    SLresult result;
    if (sampleRate >= 0) {//音频播放速率
        ad->bqPlayerSampleRate = (SLmilliHertz) sampleRate * 1000;
    }

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,
                                   1,
                                   SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_CENTER,
                                   SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if (ad->bqPlayerSampleRate) {
        format_pcm.samplesPerSec = ad->bqPlayerSampleRate;       //sample rate in mili second
    }

    format_pcm.numChannels = (SLuint32) channel;
    if (channel == 2) {
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    } else {
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    }

    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, ad->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case slplayer的接口支持参数
     */
    const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE,
                                  SL_IID_VOLUME,
                                  SL_IID_PLAYBACKRATE,
                                  SL_IID_EFFECTSEND
            /*SL_IID_MUTESOLO,*/};
    const SLboolean req[4] = {SL_BOOLEAN_TRUE,
                              SL_BOOLEAN_TRUE,
                              SL_BOOLEAN_TRUE,
                              SL_BOOLEAN_TRUE
            /*SL_BOOLEAN_TRUE,*/ };

    result = (*ad->engineEngine)->CreateAudioPlayer(ad->engineEngine, &ad->bqPlayerObject,
                                                    &audioSrc, &audioSnk,
                                                    ad->bqPlayerSampleRate ? 3 : 4, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the player
    result = (*ad->bqPlayerObject)->Realize(ad->bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the play interface
    result = (*ad->bqPlayerObject)->GetInterface(ad->bqPlayerObject, SL_IID_PLAY,
                                                 &ad->bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the buffer queue interface
    result = (*ad->bqPlayerObject)->GetInterface(ad->bqPlayerObject, SL_IID_BUFFERQUEUE,
                                                 &ad->audioPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    //get the play rate interface
    result = (*ad->bqPlayerObject)->GetInterface(ad->bqPlayerObject, SL_IID_PLAYBACKRATE,
                                                 &ad->bqPlaybackRate);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*ad->bqPlaybackRate)->SetPropertyConstraints(ad->bqPlaybackRate,
                                                           SL_RATEPROP_NOPITCHCORAUDIO);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    //user set the play rate  500～2000范围 默认 1000是正常速度
    result = (*ad->bqPlaybackRate)->SetRate(ad->bqPlaybackRate, 1000);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // register callback on the buffer queue
    result = (*ad->audioPlayerBufferQueue)->RegisterCallback(ad->audioPlayerBufferQueue,
                                                             ad->callback, ad->userdata);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the effect send interface
    ad->bqPlayerEffectSend = NULL;
    if (0 == ad->bqPlayerSampleRate) {
        result = (*ad->bqPlayerObject)->GetInterface(ad->bqPlayerObject, SL_IID_EFFECTSEND,
                                                     &ad->bqPlayerEffectSend);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*ad->bqPlayerObject)->GetInterface(ad->bqPlayerObject, SL_IID_VOLUME,
                                                 &ad->bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // set the player's state to playing
    result = (*ad->bqPlayerPlay)->SetPlayState(ad->bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
}

void shutdown(SLAudio *ad) {
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (ad->bqPlayerObject != NULL) {
        (*ad->bqPlayerObject)->Destroy(ad->bqPlayerObject);
        ad->bqPlayerObject = NULL;
        ad->bqPlayerPlay = NULL;
        ad->audioPlayerBufferQueue = NULL;
        ad->bqPlayerEffectSend = NULL;
        ad->bqPlayerMuteSolo = NULL;
        ad->bqPlayerVolume = NULL;
        ad->userdata = NULL;
    }
    // destroy engine object, and invalidate all associated interfaces
    if (ad->engineObject != NULL) {
        (*ad->engineObject)->Destroy(ad->engineObject);
        ad->engineObject = NULL;
        ad->engineEngine = NULL;
    }
}


