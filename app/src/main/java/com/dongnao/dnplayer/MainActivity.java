package com.dongnao.dnplayer;

import android.Manifest;
import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

import java.io.IOException;

/**
 * Created by xiang on 2017/9/22.
 */

public class MainActivity extends Activity {


    SurfaceView surfaceView;
    DNPlayer dnPlayer;
    EditText src;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 100);
        }
        surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        src = (EditText) findViewById(R.id.src);
        dnPlayer = new DNPlayer();
        //支持后台播放....
        dnPlayer.setDisplay(surfaceView);
    }

    public void play(View view) {
//        dnPlayer.play(src.getText().toString());
        dnPlayer.play("rtmp://live.hkstv.hk.lxdns.com/live/hks");

//        dnPlayer.play("/sdcard/a.flv");
//        MediaPlayer mediaPlayer = new MediaPlayer();
//        try {
//            mediaPlayer.setDataSource("/sdcard/a.flv");
//            mediaPlayer.setDisplay(surfaceView.getHolder());
//            mediaPlayer.prepare();
//            mediaPlayer.start();
//        } catch (IOException e) {
//            e.printStackTrace();
//        }

    }

    public void stop(View view) {
        dnPlayer.stop();
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        dnPlayer.release();
    }
}
