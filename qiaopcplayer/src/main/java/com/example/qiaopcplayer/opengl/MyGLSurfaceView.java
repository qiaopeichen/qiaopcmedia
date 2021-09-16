package com.example.qiaopcplayer.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.Surface;

public class MyGLSurfaceView extends GLSurfaceView {

    private MyRender render;

    public MyGLSurfaceView(Context context) {
        this(context, null);
    }

    public MyGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);//使用opengl 2.0版本
//        setRenderer(new MyRender(context));
        render = new MyRender(context);
        setRenderer(render);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY); //脏模式 不会频繁渲染，GLSurface里面调用requestRender的时候，就会渲染一下
        render.setOnRenderListener(new MyRender.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();
            }
        });
    }

    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (render != null) {
            render.setYUVRenderData(width, height, y, u , v);
            requestRender();
        }
    }

    public MyRender getRender() {
        return render;
    }
}
