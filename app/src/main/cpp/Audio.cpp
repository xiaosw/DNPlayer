//
// Created by liuxiang on 2017/9/24.
//


#include <pthread.h>
#include "Log.h"
#include "Audio.h"


void default_audio_call(JNIEnv *env, uint8_t *buffer, int len) {

}

static void (*audio_call)(JNIEnv *env, uint8_t *buffer, int len) =default_audio_call;

Audio::Audio() : codec(0), index(-1), clock(0), isPlay(0) {
    //重采样后的缓冲数据
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

Audio::~Audio() {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}


void *play_audio(void *args) {
    Audio *audio = (Audio *) args;
    JNIEnv *env;
    audio->vm->AttachCurrentThread(&env, 0);
    uint64_t dst_nb_samples;
    int nb;
    int data_size = 0;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    uint8_t *buff = (uint8_t *) malloc(MAX_AUDIO_FRME_SIZE * sizeof(uint8_t));
    int channels = av_get_channel_layout_nb_channels(A_CH_LAYOUT);
    //重采样
    SwrContext *swr_ctx = swr_alloc_set_opts(0, A_CH_LAYOUT, A_PCM_BITS, A_SAMPLE_RATE,
                                             audio->codec->channel_layout,
                                             audio->codec->sample_fmt,
                                             audio->codec->sample_rate, 0, 0);
    swr_init(swr_ctx);
    while (audio->isPlay) {
        audio->deQueue(packet);
        if (packet->pts != AV_NOPTS_VALUE) {
            audio->clock = av_q2d(audio->time_base) * packet->pts;
        }
        int ret = avcodec_send_packet(audio->codec, packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            LOGI("audio break");
            break;
        }
        ret = avcodec_receive_frame(audio->codec, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            LOGI("audio break");
            break;
        }
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                frame->sample_rate,
                frame->sample_rate, AVRounding(1));
        nb = swr_convert(swr_ctx, &buff, dst_nb_samples,
                         (const uint8_t **) frame->data, frame->nb_samples);
        //通道数*2(16bit)
        data_size = nb * channels * 2;
        audio_call(env, buff, data_size);
        //总子节／每秒钟音频播放的字节数 = 播放时间
        //每秒钟音频播放的字节数 = 采样*声道数*format(这里是pcm16所以占2字节)
        //播放完成这段音频后音轨的时间
        double timer = static_cast<double>(data_size) / (A_SAMPLE_RATE * channels * 2);
        audio->clock += timer;
//        av_usleep(5000);
        av_packet_unref(packet);
        av_frame_unref(frame);
    }
    LOGI("AUDIO EXIT???");
    av_packet_free(&packet);
    av_frame_free(&frame);
    free(buff);
    if (swr_ctx)
        swr_free(&swr_ctx);
    size_t size = audio->queue.size();
    for (int i = 0; i < size; ++i) {
        AVPacket *pkt = audio->queue.front();
        av_packet_free(&pkt);
        audio->queue.pop();
    }
    audio->vm->DetachCurrentThread();
    LOGI("AUDIO EXIT");
    pthread_exit(0);
}


void Audio::setPlayCall(void (*call)(JNIEnv *env, uint8_t *buffer, int len)) {
    audio_call = call;
}

void Audio::setCodec(AVCodecContext *codec) {
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    this->codec = codec;
}

double Audio::getClock() {
    return clock;
}


void Audio::play() {
    isPlay = 1;
    pthread_create(&p_playid, 0, play_audio, this);
}

void Audio::stop() {
    LOGI("AUDIO stop");
    //因为可能卡在 deQueue
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    LOGI("AUDIO lock stop");
    pthread_join(p_playid, 0);
    LOGI("AUDIO join pass");
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGI("AUDIO close");
}


int Audio::enQueue(const AVPacket *packet) {
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0) {
        return 0;
    }
    pthread_mutex_lock(&mutex);
    queue.push(pkt);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int Audio::deQueue(AVPacket *packet) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (!queue.empty()) {
            if (av_packet_ref(packet, queue.front()) < 0) {
                ret = false;
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            ret = 1;
            break;
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

