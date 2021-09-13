//
// Created by Administrator on 2020/10/20.
//

#ifndef TESTVIDEO_AUDIOCHANNEL_H
#define TESTVIDEO_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
};

class AudioChannel : public BaseChannel {
    friend void *audioPlay_t(void *);

    friend void bqPlayerCallback(SLAndroidSimpleBufferQueueItf queue, void *pContext);

private:
    bool isPlaying = 0;
    pthread_t audioDecodeTask;
    pthread_t audioPlayTask;
    SwrContext *swrContext = nullptr;

    uint8_t *buffer;
    int bufferSize;
    int out_channls;
    int out_sampleSize;

    int _getData();

    void _releaseOpenSL();

    SLObjectItf engineObject = NULL;
    SLEngineItf engineInterface = NULL;
    SLObjectItf outputMixObject = NULL;
    SLObjectItf bqPlayerObject = NULL;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;
    SLPlayItf bqPlayerInterface = NULL;
public:
    AudioChannel(int channelId, JavaCallHelper *helper,
                 AVCodecContext *avCodecContext, AVRational base);

    virtual ~AudioChannel();

    virtual void play();

    virtual void stop();

    virtual void decode();

    void _play();
};


#endif //TESTVIDEO_AUDIOCHANNEL_H
