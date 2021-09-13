//
// Created by Administrator on 2020/10/20.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int channelId, JavaCallHelper *helper,
                           AVCodecContext *avCodecContext, AVRational base)
        : BaseChannel(channelId, helper, avCodecContext, base) {
    out_channls = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    int out_sampleRate = 44100;
    bufferSize = out_sampleRate * out_sampleSize * out_channls;
    buffer = static_cast<uint8_t *>(malloc(bufferSize));
}

void *audioDecode_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

void *audioPlay_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->_play();
    return 0;
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf queue, void *pContext) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(pContext);
    int dataSize = audioChannel->_getData();
    if (dataSize > 0) {
        (*queue)->Enqueue(queue, audioChannel->buffer, dataSize);
    }
}

void AudioChannel::_play() {

    /**
     * 播放流程：
     * 1.创建引擎对象
     * 2.设置混音器
     * 3.创建播放器
     * 4.开始播放
     * 5.停止播放
     */

    engineObject = nullptr;
    SLresult result;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS) {
        LOG_E("create sl engine fail");
        return;
    }

    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);

    if (result != SL_RESULT_SUCCESS) {
        LOG_E("create sl engine fail");
        return;
    }
    // 获取引擎接口engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * step2 创建混音器
     */
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    // 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * step3:创建播放器
     */

/**
     * 3、创建播放器
     */
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式: pcm、声道数、采样率、采样位、容器大小、通道掩码(双声道)、字节序(小端)
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    //数据源 （数据获取器+格式）  从哪里获得播放数据
    SLDataSource slDataSource = {&android_queue, &pcm};

    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, nullptr};


    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    //播放器相当于对混音器进行了一层包装， 提供了额外的如：开始，停止等方法。
    // 混音器来播放声音
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //获得播放数据队列操作接口

    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueue);
    //设置回调（启动播放器后执行回调来获取数据并播放）
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    //获取播放状态接口

    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
    // 设置播放状态
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);

    //还要手动调用一次 回调方法，才能开始播放
    bqPlayerCallback(bqPlayerBufferQueue, this);

}


int AudioChannel::_getData() {
    int dataSize = 0;
    AVFrame *frame = 0;
    while (isPlaying) {
        int ret = frame_queue.deQueue(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //转换 nb= 有效转换字节数
        int nb = swr_convert(swrContext, &buffer, bufferSize,
                             (const uint8_t **) frame->data, frame->nb_samples);
        dataSize = nb * out_channls * out_sampleSize;
        //播放这一段声音的时刻
        // pts: 占多少时间刻度，
        // time_base 每个刻度的值
        // av_q2d(time_base) : 每个刻度多少秒
        //clock = 帧显示的时间戳
        clock = frame->pts * av_q2d(time_base);
        break;
    }
    releaseAvFrame(frame);
    return dataSize;
}

void AudioChannel::play() {
    //立体声
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,
                                    avCodecContext->channel_layout,
                                    avCodecContext->sample_fmt, avCodecContext->sample_rate, 0, 0);

    swr_init(swrContext);
    isPlaying = true;
    setEnable(true);
    //解码
    pthread_create(&audioDecodeTask, 0, &audioDecode_t, this);
    //播放
    pthread_create(&audioPlayTask, 0, &audioPlay_t, this);
}

void AudioChannel::stop() {
    isPlaying = 0;
    helper = 0;
    setEnable(0);
    pthread_join(audioDecodeTask, 0);
    pthread_join(audioPlayTask, 0);
    _releaseOpenSL();
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
}

void AudioChannel::_releaseOpenSL() {
    //设置停止状态
    if (bqPlayerInterface) {
        (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_STOPPED);
        bqPlayerInterface = 0;
    }
    //销毁播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueue = 0;
    }
    //销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //销毁引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }
}


void AudioChannel::decode() {
    AVPacket *avPacket = nullptr;
    while (isPlaying) {
        int ret = pkt_queue.deQueue(avPacket);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        ret = avcodec_send_packet(avCodecContext, avPacket);
        releaseAvPacket(avPacket);
        if (ret < 0) {
            break;
        }
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }

        frame_queue.enQueue(avFrame);
    }
}

AudioChannel::~AudioChannel() {

}


