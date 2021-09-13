//
// Created by Administrator on 2020/10/19.
//




#include "VideoPlayer.h"

void *prepare_t(void *args) {
    VideoPlayer *player = static_cast<VideoPlayer *>(args);
    player->_prepare();
    return 0;
}

void VideoPlayer::setDataSource(const char *_path) {

    /*path = static_cast<char*>(malloc(strlen(_path) + 1));
    memset((void*)path,0,strlen(_path) + 1);
    memcpy(path,_path,strlen(_path));*/

    path = new char[strlen(_path) + 1];
    strcpy(path, _path);
}

void VideoPlayer::prepare() {
    pthread_create(&prepareTask, 0, &prepare_t, this);
}

VideoPlayer::VideoPlayer(JavaCallHelper *_callHelper) : helper(_callHelper) {
    videoChannel = NULL;
    avformat_network_init();
}

VideoPlayer::~VideoPlayer() {
    avformat_network_deinit();
    delete helper;
    helper = 0;
    if(path) {
        delete[] path;
        path = 0;
    }
}

void VideoPlayer::_prepare() {
    avFormatContext = avformat_alloc_context();
    /**
     * step1：打开媒体文件
     */
    //para3:文件封装格式
    AVDictionary *opts;
    //超时时间
    av_dict_set(&opts, "timeout", "3000000", 0);
    int ret = avformat_open_input(&avFormatContext, path, 0, &opts);
    if (ret != 0) {
        char *error = av_err2str(ret);
        LOG_E("打开文件 %s 失败, 错误码: %d , 错误信息：%s", path, ret, error);
        helper->onError(FFMPEG_CAN_NOT_OPEN_URL, THREAD_CHILD);
        return;
    }

    /**
     * step2:查找媒体流
     */

    ret = avformat_find_stream_info(avFormatContext, 0);
    if (ret < 0) {
        LOG_E("查找媒体流 %s , 错误码: %d , 错误信息：%s", path, ret, av_err2str(ret));
        return;
    }
    duration = avFormatContext->duration / AV_TIME_BASE;
    //媒体文件中有几路流
    LOG_E("媒体文件流数目: %d", avFormatContext->nb_streams);
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        //获取一路流
        AVStream *avStream = avFormatContext->streams[i];
        //解码信息
        AVCodecParameters *parameters = avStream->codecpar;
        //查找解码器
        AVCodec *avCodec = avcodec_find_decoder(parameters->codec_id);
        if (!avCodec) {
            helper->onError(FFMPEG_FIND_DECODER_FAIL, THREAD_CHILD);
            return;
        }
        AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
        //把解码信息赋值给解码上下文的各种成员
        if (avcodec_parameters_to_context(avCodecContext, parameters) < 0) {
            helper->onError(FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL, THREAD_CHILD);
            return;
        }

        //打开解码器
        if (avcodec_open2(avCodecContext, avCodec, 0) != 0) {
            helper->onError(FFMPEG_OPEN_DECODER_FAIL, THREAD_CHILD);
        }

        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) { //音频
            audioChannel = new AudioChannel(i, helper, avCodecContext, avStream->time_base);
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {//视频
            //帧率
            double fps = av_q2d(avStream->avg_frame_rate);
            if (isnan(fps) || fps == 0) {
                fps = av_q2d(avStream->r_frame_rate);
            }
            if(isnan(fps) || fps == 0) {
                fps = av_q2d(av_guess_frame_rate(avFormatContext,avStream,0));
            }
            videoChannel = new VideoChannel(i, helper, avCodecContext, avStream->time_base, fps);
            if (!window) {
                LOG_E("初始化video channel window = null");
            }
            videoChannel->setWindow(window);
        }
    }
    //如果没有视频
    if (!videoChannel && !audioChannel) {
        helper->onError(FFMPEG_NOMEDIA, THREAD_CHILD);
        return;
    }

    helper->onPrepare(THREAD_CHILD,duration);
    return;
}

void *start_t(void *args) {
    VideoPlayer *player = static_cast<VideoPlayer *>(args);
    player->_start();
}

void VideoPlayer::start() {
    //读取媒体源数据
    //根据数据类型方式audio / videoChannel中
    isPlaying = 1;
    if (videoChannel) {
        videoChannel->audioChannel = audioChannel;
        videoChannel->play();
    }

    if (audioChannel) {
        audioChannel->play();
    }
    pthread_create(&startTask, 0, &start_t, this);
}

void VideoPlayer::_start() {
    int ret;
    while (isPlaying) {
        AVPacket *avPacket = av_packet_alloc();
        ret = av_read_frame(avFormatContext, avPacket);
        if (ret == 0) {
            if (videoChannel && avPacket->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.enQueue(avPacket);  //添加到队列
            } else if (audioChannel && avPacket->stream_index == audioChannel->channelId) {
                audioChannel->pkt_queue.enQueue(avPacket);
            } else {
                //第三路流不处理
                av_packet_free(&avPacket);
            }
        } else if (ret == AVERROR_EOF) {    //到了文件末尾
            if (videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty()) {
                //播放完毕
                break;
            }
        } else {
            break;
        }
    }
    isPlaying = false;
    videoChannel->stop();
    audioChannel->stop();
}

void VideoPlayer::setWindow(ANativeWindow *window) {
    LOG_E("EnjoyPlayer::setWindow");
    this->window = window;
    if (videoChannel) {
        LOG_E("初始化surface videoChannel->setWindow(window");
        videoChannel->setWindow(window);
    }
}

void VideoPlayer::stop() {
    isPlaying = 0;
    pthread_join(prepareTask,0);
    pthread_join(startTask,0);
    release();

}

void VideoPlayer::release() {
    if (audioChannel) {
        delete audioChannel;
        audioChannel = 0;
    }
    if (videoChannel) {
        delete videoChannel;
        videoChannel = 0;
    }
    if (avFormatContext) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = 0;
    }
}

void VideoPlayer::seek(int position) {
    if (position >= duration) {
        return;
    }

    if(!audioChannel && !videoChannel) {
        return;
    }
    if(!avFormatContext) {
        return;
    }
    isSeek = 1;
    pthread_mutex_lock(&seekMutex);

    int64_t seek = position / AV_TIME_BASE;
    //seek到请求时间之前最近的关键帧
    av_seek_frame(avFormatContext,-1,seek,AVSEEK_FLAG_BACKWARD);

    /**
     * 1.停止播放
     * 2.请客缓冲区
     * 3.重启播放
     */


    pthread_mutex_unlock(&seekMutex);
}
