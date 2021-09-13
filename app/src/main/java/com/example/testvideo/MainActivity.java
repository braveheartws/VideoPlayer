package com.example.testvideo;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final String TAG = "_ZHANGJUNPU";


    VideoPlayer player;
    SurfaceView surfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.surfaceView);
        surfaceView.getHolder().addCallback(this);
        player = new VideoPlayer();
        player.setDataSource("/sdcard/1.flv");


    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {

    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        Surface surface = surfaceHolder.getSurface();
        player.setSurface(surface);

        player.setOnPrepareListener(new VideoPlayer.OnPrepareListener() {
            @Override
            public void onPrepared(int duration) {
                Log.d(TAG, "onPrepared: duration: " + duration);
                player.start();
            }
        });
        player.prepare();

    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.stop();
    }
}
