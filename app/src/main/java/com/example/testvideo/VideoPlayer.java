package com.example.testvideo;

import android.view.Surface;

public class VideoPlayer {
    static {
        System.loadLibrary("native-lib");
    }

    private long nativeHandle;

    public VideoPlayer() {
        this.nativeHandle = nativeInit();
    }

    public void setDataSource(String path) {
        setDataSource(nativeHandle, path);
    }

    public void start() {
        start(nativeHandle);
    }

    public void stop() {
        stop(nativeHandle);
    }

    public void prepare() {
        prepare(nativeHandle);
    }

    public void seek(int position) {
        seek(nativeHandle,position);
    }


    public void setSurface(Surface surface) {
        setSurface(nativeHandle, surface);
    }


    private native void setSurface(long nativeHandle, Surface surface);

    private native long nativeInit();

    private native void setDataSource(long nativeHandle, String path);

    private native void prepare(long nativeHandle);

    private native void start(long nativeHandle);
    private native void stop(long nativeHandle);

    private native void seek(long nativeHandle,int position);

    private void onError(int errorCode) {
        if (onErrorListener != null) {
            onErrorListener.onError(errorCode);
        }
    }

    private void onPrepare(int duration) {
        if (onPrepareListener != null) {
            onPrepareListener.onPrepared(duration);
        }
    }

    private void onProgress(int progress) {
        if (onProgressListener != null) {
            onProgressListener.onProgress(progress);
        }
    }

    private OnErrorListener onErrorListener;
    private OnProgressListener onProgressListener;
    private OnPrepareListener onPrepareListener;

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setOnPrepareListener(OnPrepareListener onPrepareListener) {
        this.onPrepareListener = onPrepareListener;
    }

    public void setOnProgressListener(OnProgressListener onProgressListener) {
        this.onProgressListener = onProgressListener;
    }

    public interface OnErrorListener {
        void onError(int err);
    }

    public interface OnPrepareListener {
        void onPrepared(int maxLength);
    }

    public interface OnProgressListener {
        void onProgress(int progress);
    }
}
