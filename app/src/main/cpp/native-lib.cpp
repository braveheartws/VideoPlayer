#include <jni.h>
#include <string>
#include <android/log.h>
#include "Log.h"
#include "VideoPlayer.h"
#include "JavaCallHelper.h"

JavaVM *javaVM = 0;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_testvideo_VideoPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    VideoPlayer *videoPlayer = new VideoPlayer(new JavaCallHelper(javaVM, env, thiz));
    return reinterpret_cast<jlong>(videoPlayer);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testvideo_VideoPlayer_setDataSource(JNIEnv *env, jobject thiz, jlong native_handle,
                                                     jstring _path) {
    const char *ptr_path = env->GetStringUTFChars(_path, 0);
    VideoPlayer *player = reinterpret_cast<VideoPlayer *>(native_handle);
    player->setDataSource(ptr_path);
    env->ReleaseStringUTFChars(_path, ptr_path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testvideo_VideoPlayer_prepare(JNIEnv *env, jobject thiz, jlong native_handle) {
    VideoPlayer *player = reinterpret_cast<VideoPlayer *>(native_handle);
    player->prepare();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testvideo_VideoPlayer_start(JNIEnv *env, jobject thiz, jlong native_handle) {
    VideoPlayer* player = reinterpret_cast<VideoPlayer *>(native_handle);
    player->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testvideo_VideoPlayer_setSurface(JNIEnv *env, jobject thiz, jlong native_handle,
                                                  jobject surface) {
    VideoPlayer *player = reinterpret_cast<VideoPlayer *>(native_handle);
    ANativeWindow* window = ANativeWindow_fromSurface(env,surface);
    player->setWindow(window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testvideo_VideoPlayer_stop(JNIEnv *env, jobject thiz, jlong native_handle) {
    VideoPlayer* player = reinterpret_cast<VideoPlayer *>(native_handle);
    player->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testvideo_VideoPlayer_seek(JNIEnv *env, jobject thiz, jlong native_handle,
                                            jint position) {
    VideoPlayer* player = reinterpret_cast<VideoPlayer *>(native_handle);
    player->seek(position);
}