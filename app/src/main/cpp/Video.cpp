//
// Created by liuxiang on 2017/9/24.
//


#include <pthread.h>
#include "Video.h"

#include "Log.h"

typedef struct {
    Audio *audio;
    Video *video;
} Sync;

static const double SYNC_THRESHOLD = 0.01;
static const double NOSYNC_THRESHOLD = 10.0;

void default_video_call(AVFrame *frame) {

}

static void (*video_call)(AVFrame *frame) = default_video_call;

Video::Video() : clock(0), last_pts(0), last_delay(0), codec(0), sws_ctx(0), index(-1),
                 rgb_frame(0), isPlay(0), frame_timer(0) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&dec_mutex, NULL);
    pthread_cond_init(&dec_cond, NULL);
}

void Video::setPlayCall(void (*call)(AVFrame *frame)) {
    video_call = call;
}

void Video::setCodec(AVCodecContext *codec) {
    this->codec = codec;
    //转换rgba
    sws_ctx = sws_getContext(
            codec->width, codec->height, codec->pix_fmt,
            codec->width, codec->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);

    rgb_frame = av_frame_alloc();
    int out_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, codec->width,
                                            codec->height, 1);
    uint8_t *out_buffer = (uint8_t *) malloc(sizeof(uint8_t) * out_size);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer,
                         AV_PIX_FMT_RGBA,
                         codec->width, codec->height, 1);
}

double Video::synchronize(AVFrame *frame, double pts) {
    double frame_delay;
    //clock是当前播放的时间位置
    if (pts != 0)
        clock = pts;
    else //pst为0 则先把pts设为上一帧时间
        pts = clock;

    frame_delay = av_q2d(stream->time_base);
    //repeat_pict是解码时这帧需要延迟多少
    frame_delay += frame->repeat_pict * (frame_delay * 0.5);
    clock += frame_delay;
    return pts;
}

void *decode_video(void *args) {
    Video *video = (Video *) args;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    double pts;
    while (video->isPlay) {
        video->deQueue(packet);
        int ret = avcodec_send_packet(video->codec, packet);
        if (ret == AVERROR(EAGAIN)) {
            goto cont;
        } else if (ret < 0) {
            break;
        }
        ret = avcodec_receive_frame(video->codec, frame);
        if (ret == AVERROR(EAGAIN)) {
            goto cont;
        } else if (ret < 0) {
            break;
        }
        if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE)
            pts = 0;
        //pts 单位就是time_base
        //av_q2d转为双精度浮点数 x pts 得到pts---显示时间:秒
        pts *= av_q2d(video->stream->time_base);
        pts = video->synchronize(frame, pts);
        //将私有数据设置为pts
        frame->opaque = &pts;
        video->enQueue(frame);
        cont:
        av_packet_unref(packet);
        av_frame_unref(frame);
    }

    pthread_exit(0);
}

void *play_video(void *args) {
    Sync *sync = (Sync *) args;
    AVFrame *frame = av_frame_alloc();
    double actual_delay, current_pts, delay, ref_clock, diff, sync_threshold;
    sync->video->frame_timer = (double) av_gettime() / 1000000.0;
    while (sync->video->isPlay) {
        sync->video->deQueue(frame);
        sws_scale(sync->video->sws_ctx,
                  (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height,
                  sync->video->rgb_frame->data, sync->video->rgb_frame->linesize);
        video_call(sync->video->rgb_frame);

        //当前帧pts
        current_pts = *(double *) frame->opaque;
        //当前帧与上一帧的时间差
        delay = current_pts - sync->video->last_pts;
        if (delay <= 0 || delay >= 1.0) {
            //如果延误不正确 太高或太低，使用上一个
            delay = sync->video->last_delay;
        }
        LOGI("delay %f", delay);
        sync->video->last_delay = delay;
        sync->video->last_pts = current_pts;
        ref_clock = sync->audio->getClock();
        LOGI("ref_clock %f", ref_clock);
        LOGI("current pts %f", current_pts);
        //大于0就是视频快了,小于就是音频快了
        diff = current_pts - ref_clock;
        LOGI("diff %f", diff);
        sync_threshold =
                (delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;
        //防止没有音频 如果误差在10以内则进行同步，否则认为没音频
        if (fabs(diff) < NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold) {
                delay = 0;
                LOGI("加快速度");
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
                LOGI("减慢速度 %f", delay);
            }
        }
        LOGI("frame_timer1 %f", sync->video->frame_timer);
        sync->video->frame_timer += delay;
        LOGI("frame_timer2 %f", sync->video->frame_timer);
        LOGI("current_timer %f", (av_gettime() / 1000000.0));
        actual_delay = sync->video->frame_timer - (av_gettime() / 1000000.0);
        LOGI("actual_delay %f", actual_delay);
        if (actual_delay < 0.010) {
            actual_delay = 0.010;
        }
        LOGI("sleep %f", (actual_delay * 1000));
        av_usleep(actual_delay * 1000+0.5);

        av_frame_unref(frame);

    }
    pthread_exit(0);
}

void Video::play(Audio *audio) {
    Sync *sync = (Sync *) malloc(sizeof(Sync));
    sync->audio = audio;
    sync->video = this;
    isPlay = 1;
    pthread_create(&p_decid, 0, decode_video, this);
    pthread_create(&p_playid, 0, play_video, sync);
}

void Video::stop() {
    isPlay = 0;
    pthread_join(p_decid, 0);
    pthread_join(p_playid, 0);
}

int Video::enQueue(const AVFrame *frame) {
    AVFrame *f = av_frame_alloc();
    if (av_frame_ref(f, frame) < 0)
        return 0;
    pthread_mutex_lock(&mutex);
    queue.push(f);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int Video::deQueue(AVFrame *frame) {
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


int Video::enQueue(const AVPacket *packet) {
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0)
        return 0;
    pthread_mutex_lock(&dec_mutex);
    dec_queue.push(pkt);
    pthread_cond_signal(&dec_cond);
    pthread_mutex_unlock(&dec_mutex);
    return 1;
}

int Video::deQueue(AVPacket *packet) {
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