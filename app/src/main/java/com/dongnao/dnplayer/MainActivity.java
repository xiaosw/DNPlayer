package com.dongnao.dnplayer;

import android.Manifest;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;

/**
 * Created by xiang on 2017/9/22.
 * 动脑学院 版权所有
 */

public class MainActivity extends Activity {



    SurfaceView surfaceView;
    DNPlayer dnPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 100);
        }
        surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        dnPlayer = new DNPlayer();
        dnPlayer.setDisplay(surfaceView);
    }

    public void play(View view) {
        dnPlayer.play("rtmp://live.hkstv.hk.lxdns.com/live/hks");

    }


}
