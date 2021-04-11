package com.lake.ffmpeg421demo;

public interface LPlayCallback {
    void videoInfo(double time);
    void onProgress(double pos);
}
