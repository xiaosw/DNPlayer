package com.dongnao.dnplayer;

import android.Manifest;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

/**
 * Created by xiang on 2017/9/22.
 * 动脑学院 版权所有
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
        dnPlayer.play(src.getText().toString());

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
