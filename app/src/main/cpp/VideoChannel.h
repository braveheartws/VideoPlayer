//
// Created by Administrator on 2020/10/20.
//

#ifndef TESTVIDEO_VIDEOCHANNEL_H
#define TESTVIDEO_VIDEOCHANNEL_H


#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <android/native_window.h>
    #include <libavutil/time.h>
}


class VideoChannel : public BaseChannel {
    friend void *videoPlay_t(void *);

public:
    VideoChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext, AVRational
    base, double fps);

    virtual ~VideoChannel();

    virtual void play();

    virtual void stop();

    virtual void decode();

    void setWindow(ANativeWindow *pWindow);

private:
    void _play();

private:
    double fps;
    pthread_mutex_t surfaceMutex;
    pthread_t videoDecodeTask;
    pthread_t videoPlayTask;
    bool isPlaying;
    ANativeWindow *window = nullptr;
public:
    AudioChannel* audioChannel = nullptr;

    void draw(uint8_t *pString[4], int pInt[4], int width, int height);
};


#endif //TESTVIDEO_VIDEOCHANNEL_H
