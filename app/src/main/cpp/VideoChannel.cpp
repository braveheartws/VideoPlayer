//
// Created by Administrator on 2020/10/20.
//


#include "VideoChannel.h"

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1

VideoChannel::VideoChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                           AVRational base, double fps) : BaseChannel(channelId, helper,
                                                                      avCodecContext, base),
                                                          fps(fps) {
    pthread_mutex_init(&surfaceMutex, 0);
}

void *videoDecode_t(void *args) {
    VideoChannel *videoChannel = reinterpret_cast<VideoChannel *>(args);
    videoChannel->decode();
    return 0;
}

void *videoPlay_t(void *args) {
    VideoChannel *videoChannel = reinterpret_cast<VideoChannel *>(args);
    videoChannel->_play();
    return 0;
}

void VideoChannel::play() {
    isPlaying = 1;
    //开启队列的工作
    setEnable(true);
    //解码
    pthread_create(&videoDecodeTask, 0, &videoDecode_t, this);

    //播放
    pthread_create(&videoPlayTask, 0, videoPlay_t, this);
}

void VideoChannel::stop() {
    isPlaying = 0;
    helper = 0;
    setEnable(0);
    pthread_join(videoDecodeTask,0);
    pthread_join(videoPlayTask,0);
    if(window) {
        ANativeWindow_release(window);
        window = 0;
    }
}

void VideoChannel::_play() {
    LOG_E("_play");
    //只做缩放与格式转换
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, 0, 0, 0);
    uint8_t *data[4];
    int linesize[4];
    av_image_alloc(data, linesize, avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA,
                   1);
    AVFrame *avFrame = 0;
    int ret;
    double frame_delay = 1.0 / fps;
    while (isPlaying) {
        ret = frame_queue.deQueue(avFrame);
        LOG_E("取出 avframe 数据");
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }


        // 让视频流畅播放
        double extra_delay = avFrame->repeat_pict / (2 * fps);
        double delay = extra_delay + frame_delay;

        if (audioChannel) {
            clock = avFrame->best_effort_timestamp * av_q2d(time_base);
            double diff = clock - audioChannel->clock;
            //允许有误差 ijkplayer 允许误差在0.04 - 0.1之间 则不用特意去做音视频同步
            /**
             * 1、delay < 0.04, 同步阈值就是0.04
             * 2、delay > 0.1 同步阈值就是0.1
             * 3、0.04 < delay < 0.1 ，同步阈值是delay
             */
            // 根据每秒视频需要播放的图象数，确定音视频的时间差允许范围
            // 给到一个时间差的允许范围
            double sync = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
            //视频落后了，落后太多了，我们需要去同步
            if (diff <= -sync) {
                //就要让delay时间减小
                delay = FFMAX(0, delay + diff);
            } else if (diff > sync) {
                //视频快了,让delay久一些，等待音频赶上来
                delay = delay + diff;
            }
            LOG_E("Video:%lf Audio:%lf delay:%lf A-V=%lf", clock, audioChannel->clock, delay, -diff);
        }

        av_usleep(delay * 1000000);

        /**
         * 2.指针数组 RGBA 每个维度的数据对应一个指针
         * 3.每一行数据大小
         * 4.offset
         * 5.要转换图像的高
         */
        LOG_E("_play# 缩放与格式转换");
        sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avFrame->height, data, linesize);
        draw(data, linesize, avCodecContext->width, avCodecContext->height);
        releaseAvFrame(avFrame);    //一定要释放内存，避免内存溢出
    }

    //release
    av_free(&data[0]);
    isPlaying = 0;
    releaseAvFrame(avFrame);
    sws_freeContext(swsContext);
}

void VideoChannel::draw(uint8_t **data, int *linesize, int width, int height) {
    LOG_E(" draw ");
    pthread_mutex_lock(&surfaceMutex);
    if (!window) {
        pthread_mutex_unlock(&surfaceMutex);
    }
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buffer;
    //绑定窗口与数据缓冲
    if (ANativeWindow_lock(window, &buffer, 0) != 0) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&surfaceMutex);
        return;
    }
    uint8_t *dstData = static_cast<uint8_t *>(buffer.bits);
    int dstSize = buffer.stride * 4;
    uint8_t *srcData = data[0];
    int srcSize = linesize[0];


    //把视频数据刷到buffer中
    for (int i = 0; i < buffer.height; i++) {
        memcpy(dstData + i * dstSize, srcData + i * srcSize, srcSize);
    }


    ANativeWindow_unlockAndPost(window);


    pthread_mutex_unlock(&surfaceMutex);
}

void VideoChannel::decode() {
    LOG_E("decode");
    AVPacket *avPacket = nullptr;
    while (isPlaying) {
        //两种阻塞状态  1. 能够读取到数据 2. 停止播放
        int ret = pkt_queue.deQueue(avPacket);
        LOG_E("取出 avpacket pkt_queue size: %d", pkt_queue.size());
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        if (avPacket == nullptr) {
            LOG_E("avPacket == NULL");
        }
        if (avCodecContext == NULL) {
            LOG_E("avCodecContext == NULL");
        }
        ret = avcodec_send_packet(avCodecContext, avPacket);
        releaseAvPacket(avPacket);
        /*if (ret < 0) {
            LOG_E("decode# 取出avpacket 添加到 avcodecContext失败");
            break;
        }*/
        LOG_E("decode# 取出avpacket 添加到 avcodecContext 成功");
        //从解码器取出解码好的数据
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        LOG_E("decode 从解码器取出解码好的数据");
        if (ret == AVERROR(EAGAIN)) {   //当前的包不够解码
            continue;
        } else if (ret < 0) {
            break;
        }
        frame_queue.enQueue(avFrame);
        LOG_E("将avpacket从队列取出，并通过avcodecContext 拿出 avFrame 添加到frame_queue");
    }
    releaseAvPacket(avPacket);
}

void VideoChannel::setWindow(ANativeWindow *_window) {
    LOG_E("VideoChannel::setWindow");
    //加锁
    pthread_mutex_lock(&surfaceMutex);
    if (window) {
        ANativeWindow_release(window);
    }
    this->window = _window;
    LOG_E("VideoChannel::this->window = _window;");
    pthread_mutex_unlock(&surfaceMutex);
}

VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&surfaceMutex);
}


