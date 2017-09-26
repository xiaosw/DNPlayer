package com.dongnao.dnplayer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by xiang on 2017/9/22.
 * 动脑学院 版权所有
 */

public class DNPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("native-lib");
    }

    private static AudioTrack audioTrack;
    private SurfaceView display;

    private native void native_play(String path);

    private native void native_set_display(Surface surface);

    private native void native_stop();

    private native void native_release();


    public void play(String path) {
        if (null == display) {
            return;
        }
        createAudioTrack();
        native_play(path);
    }

    public void stop() {
        releaseAudio();
        native_stop();
    }

    public void release() {
        stop();
        native_release();
    }


    public void setDisplay(SurfaceView display) {
        if (null != this.display) {
            this.display.getHolder().removeCallback(this);
        }
        this.display = display;
        native_set_display(display.getHolder().getSurface());
        this.display.getHolder().addCallback(this);
    }

    private void createAudioTrack() {
        releaseAudio();
        int bufferSizeInBytes = AudioTrack.getMinBufferSize(44100, AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                44100, AudioFormat.CHANNEL_OUT_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSizeInBytes, AudioTrack.MODE_STREAM);
        audioTrack.play();
    }

    public void playTrack(byte[] buffer, int len) {
        if (null != audioTrack && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(buffer, 0, len);
        }
    }


    private void releaseAudio() {
        if (null != audioTrack) {
            if (audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING)
                audioTrack.stop();
            audioTrack.release();
            audioTrack = null;
        }
    }


    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        native_set_display(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
