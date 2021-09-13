//
// Created by Administrator on 2020/10/19.
//

#ifndef TESTVIDEO_LOG_H
#define TESTVIDEO_LOG_H

#include <android/log.h>

#define LOG_E(...)  __android_log_print(ANDROID_LOG_ERROR,"FFMPEG",__VA_ARGS__)

#define LOG_D(...)  __android_log_print(ANDROID_LOG_DEBUG,"FFMPEG",__VA_ARGS__)


#endif //TESTVIDEO_LOG_H
