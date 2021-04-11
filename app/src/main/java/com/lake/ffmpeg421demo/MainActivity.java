package com.lake.ffmpeg421demo;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.WindowManager;
import android.widget.SeekBar;
import android.widget.TextView;
import java.io.File;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_PERMISSIONS_CODE = 0;
    public static final String[] PERMISSIONS = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };
    LPlayView glview;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (!checkSelfPermission()) {
            requestPermissions(PERMISSIONS, REQUEST_PERMISSIONS_CODE);
        } else {
            initView();
        }
    }

    private void initView() {
        glview = findViewById(R.id.glview);
        TextView pro = findViewById(R.id.progress);
        final SeekBar seekBar = findViewById(R.id.seekBar);
        seekBar.setMax(1000);
        File m = getExternalFilesDir(Environment.DIRECTORY_MOVIES);
        Log.e("lake", "m=" + m.getAbsolutePath());
        final String inputurl = m.getAbsolutePath() + "/xiaochou.mkv";
        File file = new File(inputurl);

        findViewById(R.id.pause).setOnClickListener(v -> {
            glview.pause();
        });

        findViewById(R.id.stop).setOnClickListener(v->{
            glview.stop();
        });

        findViewById(R.id.play).setOnClickListener(v->{
            glview.play(inputurl);
        });

        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                double pos = (double) seekBar.getProgress() / 1000L;
                glview.seek(pos);
            }
        });

        glview.setPlayProgressListener(new LPlayCallback() {
            @Override
            public void videoInfo(double time) {

            }

            @Override
            public void onProgress(final double pos) {
                Log.e("progress=%.3f", "进度" + pos * 100);
                runOnUiThread(() -> {
                    pro.setText(String.format("%.2f/100", (float) pos * 100));
                    int p = (int) (pos * 1000);
                    seekBar.setProgress(p);
                });
            }
        });
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (REQUEST_PERMISSIONS_CODE == requestCode) {
            for (int grant : grantResults) {
                if (grant != PackageManager.PERMISSION_GRANTED) {
                    return;
                }
            }
            initView();
        }
    }

    private boolean checkSelfPermission() {
        for (String s : PERMISSIONS) {
            if (checkSelfPermission(s) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }
}
