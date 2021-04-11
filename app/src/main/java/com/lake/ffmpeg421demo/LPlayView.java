package com.lake.ffmpeg421demo;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class LPlayView extends GLSurfaceView implements GLSurfaceView.Renderer {
    private static final String TAG = "LPlayView";
    private static final boolean DEBUG = true;

    public LPlayView(Context context) {
        this(context, null);
    }

    public LPlayView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(2);
        setRenderer(this);
        setRenderMode(RENDERMODE_WHEN_DIRTY);//主动刷新
    }

    /**
     * 手动刷新
     */
    public void updateFrame(){
        requestRender();
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        Log.e("gle", "onDrawFrame:");
        native_renderOnDrawFrame();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.e("gle", "onSurfaceChanged:");
        native_renderOnSurfaceChanged(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.e("gle", "onSurfaceCreated:");
        native_renderOnSurfaceCreated();
    }

    public void onRelease(){
        native_renderUnInit();
    }

    public void stop(){
        native_stop();
    }

    public void play(String path){
        native_play(path);
    }

    public void seek(double pos){
        native_doSeek(pos);
    }

    public void pause(){
        native_togglePause();
    }

    public void setPlayProgressListener(LPlayCallback callback){
        native_setPlayCallback(callback);
    }

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("lplay");
    }

    private native int native_play(String input_jstr);

    private native int native_stop();

    private native int native_togglePause();

    private native int native_doSeek(double frac);

    private native void native_setPlayCallback(LPlayCallback callback);

    private native void native_renderUnInit();

    private native void native_renderOnSurfaceCreated();

    private native void native_renderOnSurfaceChanged(int width, int height);

    private native void native_renderOnDrawFrame();
}
