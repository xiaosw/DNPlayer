//
// Created by liuxiang on 2017/9/24.
//

#ifndef DNPLAYER_AUDIO_H
#define DNPLAYER_AUDIO_H

#include <android/native_window_jni.h>
#include <queue>

extern "C" {

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>


#define MAX_AUDIO_FRME_SIZE 48000 * 4
#define A_CH_LAYOUT AV_CH_LAYOUT_STEREO
#define A_PCM_BITS AV_SAMPLE_FMT_S16
#define A_SAMPLE_RATE 44100


class Audio {
public:
    Audio();

    int enQueue(const AVFrame *frame);

    int deQueue(AVFrame *frame);

    int enQueue(const AVPacket *packet);

    int deQueue(AVPacket *packet);

    void setPlayCall(void (*call)(JNIEnv *env, uint8_t *buffer, int len));

    void play();

    void stop();

    void setCodec(AVCodecContext *codec);

    void setJvm(JavaVM *vm);

    double getClock();

public:
    AVCodecContext *codec;
    SwrContext *swr_ctx;
    int index;
    AVStream *stream;
    double clock;
    int isPlay;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    std::queue<AVFrame *> queue;

    pthread_mutex_t dec_mutex;
    pthread_cond_t dec_cond;
    std::queue<AVPacket *> dec_queue;

    pthread_t p_decid;
    pthread_t p_playid;
    JavaVM *vm;

    double frame_timer;

};
}
#endif //DNPLAYER_AUDIO_H
