//
// Created by Administrator on 2020/10/20.
//

#include "JavaCallHelper.h"


JavaCallHelper::JavaCallHelper(JavaVM *_vm, JNIEnv *_env, jobject &_obj) : javaVm(_vm), env(_env) {
    javaPlayer = env->NewGlobalRef(_obj);
    jclass jclazz = env->GetObjectClass(_obj);
    jmid_error = env->GetMethodID(jclazz, "onError", "(I)V");
    jmid_prepare = env->GetMethodID(jclazz, "onPrepare", "(I)V");
    jmid_progress = env->GetMethodID(jclazz, "onProgress", "(I)V");
}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(javaPlayer);
    javaPlayer = 0;
}

void JavaCallHelper::onError(int code, int thread) {
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(javaPlayer, jmid_error, code);
    } else {
        JNIEnv *_env;   //子线程env
        if (javaVm->AttachCurrentThread(&_env, 0) != JNI_OK) {
            return;
        }
        _env->CallVoidMethod(javaPlayer, jmid_error, code);
        javaVm->DetachCurrentThread();
    }
}

void JavaCallHelper::onPrepare(int thread,int duration) {
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(javaPlayer, jmid_prepare);
    } else {
        JNIEnv *_env;
        if (javaVm->AttachCurrentThread(&_env, 0) != JNI_OK) {
            return;
        }
        _env->CallVoidMethod(javaPlayer, jmid_prepare,duration);
        javaVm->DetachCurrentThread();
    }
}

void JavaCallHelper::onProgress(int progress, int thread) {
    if (thread == THREAD_MAIN) {
        env->CallVoidMethod(javaPlayer, jmid_progress, progress);
    } else {
        JNIEnv *_env;
        if (javaVm->AttachCurrentThread(&_env, 0) != JNI_OK) {
            return;
        }
        _env->CallVoidMethod(javaPlayer, jmid_progress, progress);
        javaVm->DetachCurrentThread();
    }
}