//
// Created by liuxiang on 2017/9/24.
//


#include <pthread.h>
#include "Log.h"
#include "Audio.h"


void default_audio_call(JNIEnv *env, uint8_t *buffer, int len) {

}

static void (*audio_call)(JNIEnv *env, uint8_t *buffer, int len) =default_audio_call;

Audio::Audio() : codec(0), stream(0), swr_ctx(0), index(-1),
                 clock(0), isPlay(0), frame_timer(0) {
    //重采样后的缓冲数据
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&dec_mutex, NULL);
    pthread_cond_init(&dec_cond, NULL);

}


void *decode_audio(void *args) {
    Audio *audio = (Audio *) args;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    while (audio->isPlay) {
        audio->deQueue(packet);
        if (packet->pts != AV_NOPTS_VALUE) {
            audio->clock = av_q2d(audio->stream->time_base) * packet->pts;
        }
        int ret = avcodec_send_packet(audio->codec, packet);
        if (ret == AVERROR(EAGAIN)) {
            goto cont;
        } else if (ret < 0) {
            break;
        }
        ret = avcodec_receive_frame(audio->codec, frame);
        if (ret == AVERROR(EAGAIN)) {
            goto cont;
        } else if (ret < 0) {
            break;
        }
        audio->enQueue(frame);
        cont:
        av_packet_unref(packet);
        av_frame_unref(frame);
    }

    pthread_exit(0);
}


void Audio::setPlayCall(void (*call)(JNIEnv *env, uint8_t *buffer, int len)) {
    audio_call = call;
}

void Audio::setCodec(AVCodecContext *codec) {
    this->codec = codec;
    //重采样
    swr_ctx = swr_alloc_set_opts(0, A_CH_LAYOUT, A_PCM_BITS, A_SAMPLE_RATE,
                                 codec->channel_layout,
                                 codec->sample_fmt,
                                 codec->sample_rate, 0, 0);
    swr_init(swr_ctx);
}

double Audio::getClock() {
    //开始播放与现在的时间差
//    double current = static_cast<double>(av_gettime()) / 1000000;
//    double c = current - frame_timer;
    return clock;
}

void *play_audio(void *args) {
    Audio *audio = (Audio *) args;
    JNIEnv *env;
    audio->vm->AttachCurrentThread(&env, 0);
    AVFrame *frame = av_frame_alloc();
    uint8_t *buff = (uint8_t *) malloc(MAX_AUDIO_FRME_SIZE * sizeof(uint8_t));
    int data_size = 0;
    int channels = av_get_channel_layout_nb_channels(A_CH_LAYOUT);
    uint64_t dst_nb_samples;
    int nb;

    while (audio->isPlay) {
        audio->deQueue(frame);
        //swr_get_delay代表的是下一输入数据与下一输出数据之间的时间间隔
        //即转换需要的时间
        //a * b / c
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(audio->swr_ctx, frame->sample_rate) + frame->nb_samples,
                frame->sample_rate,
                frame->sample_rate, AVRounding(1));
        nb = swr_convert(audio->swr_ctx, &buff, dst_nb_samples,
                         (const uint8_t **) frame->data, frame->nb_samples);
        data_size = frame->channels * nb * av_get_bytes_per_sample(A_PCM_BITS);
        //总子节／每秒钟音频播放的字节数 = 播放时间
        //每秒钟音频播放的字节数 = 采样*声道数*format(这里是pcm16所以占2字节)
        //播放完成这段音频后音轨的时间
        audio_call(env, buff, data_size);
        audio->clock += static_cast<double>(data_size) / (A_SAMPLE_RATE * channels * 2);
    }
    av_frame_free(&frame);
    free(buff);
    audio->vm->DetachCurrentThread();
    pthread_exit(0);
}

void Audio::play() {
    pthread_create(&p_decid, 0, decode_audio, this);
    isPlay = 1;
    pthread_create(&p_playid, 0, play_audio, this);
}

void Audio::stop() {
    isPlay = 0;
    pthread_join(p_decid, 0);
    pthread_join(p_playid, 0);
}

int Audio::enQueue(const AVFrame *frame) {
    AVFrame *f = av_frame_alloc();
    if (av_frame_ref(f, frame) < 0)
        return 0;
    pthread_mutex_lock(&mutex);
    queue.push(f);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int Audio::deQueue(AVFrame *frame) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    while (true) {
        if (!queue.empty()) {
            if (av_frame_ref(frame, queue.front()) < 0) {
                ret = false;
                break;
            }
            AVFrame *f = queue.front();
            queue.pop();
            av_frame_unref(f);
            av_frame_free(&f);
            ret = 1;
            break;
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}


int Audio::enQueue(const AVPacket *packet) {
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0) {
        return 0;
    }
    pthread_mutex_lock(&dec_mutex);
    dec_queue.push(pkt);
    pthread_cond_signal(&dec_cond);
    pthread_mutex_unlock(&dec_mutex);
    return 1;
}

int Audio::deQueue(AVPacket *packet) {
    int ret = 0;
    pthread_mutex_lock(&dec_mutex);
    while (true) {
        if (!dec_queue.empty()) {
            if (av_packet_ref(packet, dec_queue.front()) < 0) {
                ret = false;
                break;
            }
            AVPacket *pkt = dec_queue.front();
            dec_queue.pop();
            av_packet_unref(pkt);
            av_packet_free(&pkt);
            ret = 1;
            break;
        } else {
            pthread_cond_wait(&dec_cond, &dec_mutex);
        }
    }
    pthread_mutex_unlock(&dec_mutex);
    return ret;
}

void Audio::setJvm(JavaVM *vm) {
    this->vm = vm;
}
