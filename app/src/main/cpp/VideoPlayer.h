//
// Created by Administrator on 2020/10/19.
//

#ifndef TESTVIDEO_VIDEOPLAYER_H
#define TESTVIDEO_VIDEOPLAYER_H
extern "C" {
    #include <libavformat/avformat.h>
    #include <android/native_window_jni.h>
}
#include <malloc.h>
#include <cstring>
#include <pthread.h>

#include "Log.h"
#include "JavaCallHelper.h"
#include "VideoChannel.h"
#include "AudioChannel.h"

class VideoPlayer {
    friend void *prepare_t(void *);

    friend void *start_t(void *args);

private:
    char *path;
    pthread_t prepareTask;
    JavaCallHelper *helper = NULL;
    int64_t duration;
    VideoChannel *videoChannel = nullptr;
    AudioChannel *audioChannel = nullptr;
    bool isPlaying;
    pthread_t startTask;
    AVFormatContext *avFormatContext = NULL;
    ANativeWindow *window = nullptr;
    bool isSeek;
    pthread_mutex_t seekMutex;

private:
    void _start();

    void _prepare();

public:
    VideoPlayer(JavaCallHelper *javaCallHelper);

    ~VideoPlayer();

    void prepare();

    void setDataSource(const char *string);

    void start();

    void stop();

    void release();


    void setWindow(ANativeWindow* window);

    void seek(jint i);
};


#endif //TESTVIDEO_VIDEOPLAYER_H
