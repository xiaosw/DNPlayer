#include <jni.h>
#include <string>

#include "Log.h"

#include "Video.h"
#include "Audio.h"


extern "C" {


Video *video = 0;
Audio *audio = 0;

ANativeWindow *window = 0;

char *path = 0;
JavaVM *vm = 0;
jobject player = 0;


void call_audio_play(JNIEnv *env, uint8_t *buffer, int out_buffer_size) {
    jclass clazz = env->GetObjectClass(player);
    jmethodID play_track = env->GetMethodID(clazz, "playTrack", "([BI)V");
    jbyteArray array = env->NewByteArray(out_buffer_size);
    env->SetByteArrayRegion(array, 0, out_buffer_size, (const jbyte *) buffer);
    env->CallVoidMethod(player, play_track, array, out_buffer_size);
    env->DeleteLocalRef(array);
}
void call_video_play(AVFrame *frame) {
    if (!window) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm_, void *reserved) {
    vm = vm_;
    JNIEnv *env = NULL;
    jint result = -1;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}

void *process(void *args) {
    av_register_all();
    avformat_network_init();
    AVFormatContext *ifmt_ctx = 0;
    LOGI("set url:%s", path);
    avformat_open_input(&ifmt_ctx, path, 0, 0);
    avformat_find_stream_info(ifmt_ctx, 0);
    //查找流
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {

        AVCodecParameters *parameters = ifmt_ctx->streams[i]->codecpar;
        AVCodec *dec = avcodec_find_decoder(parameters->codec_id);

        AVCodecContext *codec = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(codec, parameters);
        avcodec_open2(codec, dec, 0);

        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio->setCodec(codec);
            audio->index = i;
            audio->stream = ifmt_ctx->streams[i];
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            video->setCodec(codec);
            video->index = i;
            video->stream = ifmt_ctx->streams[i];
            if (window)
                ANativeWindow_setBuffersGeometry(window, video->codec->width,
                                                 video->codec->height,
                                                 WINDOW_FORMAT_RGBA_8888);
        }
    }

    video->play(audio);
    audio->play();
    AVPacket *packet = av_packet_alloc();

    while (av_read_frame(ifmt_ctx, packet) == 0) {
        if (video && packet->stream_index == video->index) {
            video->enQueue(packet);
        } else if (audio && packet->stream_index == audio->index) {
            audio->enQueue(packet);

        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    audio->stop();
    video->stop();
    pthread_exit(0);
}

JNIEXPORT void JNICALL
Java_com_dongnao_dnplayer_DNPlayer_native_1play(
        JNIEnv *env,
        jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    player = env->NewGlobalRef(instance);

    path = (char *) malloc(strlen(url) + 1);
    memset(path, 0, strlen(url) + 1);
    memcpy(path, url, strlen(url));

    video = new Video;
    audio = new Audio;
    audio->setPlayCall(call_audio_play);
    audio->setJvm(vm);
    video->setPlayCall(call_video_play);

    pthread_t p_tid;
    pthread_create(&p_tid, 0, process, 0);
    env->ReleaseStringUTFChars(url_, url);
}


JNIEXPORT void JNICALL
Java_com_dongnao_dnplayer_DNPlayer_native_1set_1display(JNIEnv *env, jobject instance,
                                                        jobject surface) {

    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    if (video && video->codec)
        ANativeWindow_setBuffersGeometry(window, video->codec->width,
                                         video->codec->height,
                                         WINDOW_FORMAT_RGBA_8888);
}

}