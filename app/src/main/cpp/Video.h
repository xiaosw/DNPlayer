//
// Created by liuxiang on 2017/9/24.
//

#ifndef DNPLAYER_VIDEO_H
#define DNPLAYER_VIDEO_H

#include <android/native_window_jni.h>
#include <queue>
#include "Audio.h"


extern "C" {

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>


class Video {
public:
    Video();

    int enQueue(const AVFrame *frame);

    int deQueue(AVFrame *frame);

    int enQueue(const AVPacket *packet);

    int deQueue(AVPacket *packet);

    void play(Audio *audio);

    void stop();

    void setCodec(AVCodecContext *codec);

    void setPlayCall(void (*call)(AVFrame *frame));

    double synchronize(AVFrame *frame, double pts);

public:

    double clock;
    double last_pts;
    double last_delay;
    int isPlay;
    AVStream *stream;
    AVCodecContext *codec;
    SwsContext *sws_ctx;
    int index;
    AVFrame *rgb_frame;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    std::queue<AVFrame *> queue;

    pthread_mutex_t dec_mutex;
    pthread_cond_t dec_cond;
    std::queue<AVPacket *> dec_queue;

    pthread_t p_decid;
    pthread_t p_playid;

    double frame_timer;
};
}
#endif //DNPLAYER_VIDEO_H
