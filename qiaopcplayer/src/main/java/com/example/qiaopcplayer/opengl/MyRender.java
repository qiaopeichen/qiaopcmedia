package com.example.qiaopcplayer.opengl;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.view.Surface;

import com.example.qiaopcplayer.R;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MyRender implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {

    public static final int RENDER_YUV = 1;
    public static final int RENDER_MEDIACODEC = 2;
    protected Context context;

    //1 绘制坐标范围
    private final float[] vertexData = {
//            -1f,0f,
//            0f, -1f,
//            0f, 1f,
//            0f, 1f,
//            0f, -1f,
//            1f, 0f

            -1f, -1f,
            1f, -1f,
            -1f, 1f,
            1f, 1f
    };

    private final float[] textureData = {
            //正向
            0f, 1f,
            1f, 1f,
            0f, 0f,
            1f, 0f

            // 180度旋转
//            1f, 0f,
//            0f, 0f,
//            1f, 1f,
//            0f, 1f
    }; //纹理坐标

    //2 为坐标分配本地内存地址
    private FloatBuffer vertexBuffer;
    private FloatBuffer textureBuffer;
    private int renderType = RENDER_YUV;
    private OnSurfaceCreateListener onSurfaceCreateListener;
    private OnRenderListener onRenderListener;

    //yuv
    private int program_yuv;
    private int avPosition_yuv;
    private int afPosition_yuv;
//    private int sTexture;
    private int textureId;
    private int afColor;

    private int sampler_y;
    private int sampler_u;
    private int sampler_v;
    private int[] textureId_yuv;

    private int width_yuv;
    private int height_yuv;
    private ByteBuffer y;
    private ByteBuffer u;
    private ByteBuffer v;

    //mediacodec
    private int program_mediacodec;
    private int avPosition_mediacodec;
    private int afPosition_mediacodec;
    private int samplerOES_mediacodec;
    private int textureId_mediacodec;
    private SurfaceTexture surfaceTexture;
    private Surface surface;

    public MyRender(Context context) {
        this.context = context;
        //2 为坐标分配本地内存地址
        vertexBuffer = ByteBuffer.allocateDirect(vertexData.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(vertexData);
        vertexBuffer.position(0);
        textureBuffer = ByteBuffer.allocateDirect(textureData.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer()
                .put(textureData);
        textureBuffer.position(0);
    }

    public void setRenderType(int renderType) {
        this.renderType = renderType;
    }

    public void setOnSurfaceCreateListener(OnSurfaceCreateListener onSurfaceCreateListener) {
        this.onSurfaceCreateListener = onSurfaceCreateListener;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        initRenderYUV();
        initRenderMediacodec();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height); //设置view宽高
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT); //把颜色缓冲区清屏
        GLES20.glClearColor(0f, 0f, 0f, 1.0f); // 用颜色来清屏
        if (renderType == RENDER_YUV) {
            renderYUV();
        } else if (renderType == RENDER_MEDIACODEC) {
            renderMediacodec();
        }
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }

    private void renderMediacodec() {
        surfaceTexture.updateTexImage();
        GLES20.glUseProgram(program_mediacodec);

        GLES20.glEnableVertexAttribArray(avPosition_mediacodec); //10使顶点属性数组有效
        GLES20.glVertexAttribPointer(avPosition_mediacodec, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer); //11为顶点属性赋值
//            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4); //12 绘制图形

        GLES20.glEnableVertexAttribArray(afPosition_mediacodec); //绘制纹理坐标
        GLES20.glVertexAttribPointer(afPosition_mediacodec, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId_mediacodec);
        GLES20.glUniform1i(samplerOES_mediacodec, 0);
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        if (onRenderListener != null) {
            onRenderListener.onRender();
        }
    }

    public interface OnRenderListener {
        void onRender();
    }

    public void setOnRenderListener(OnRenderListener onRenderListener) {
        this.onRenderListener = onRenderListener;
    }

    private void initRenderYUV() {
        String vertexSource = MyShaderUtil.readRawTxt(context, R.raw.vertex_shader);
        String fragmentSource = MyShaderUtil.readRawTxt(context, R.raw.fragment_yuv);
        program_yuv = MyShaderUtil.createProgram(vertexSource, fragmentSource);
        avPosition_yuv = GLES20.glGetAttribLocation(program_yuv, "av_Position"); //8 得到着色器中的属性 顶点坐标
        afPosition_yuv = GLES20.glGetAttribLocation(program_yuv, "af_Position"); //纹理坐标

        sampler_y = GLES20.glGetUniformLocation(program_yuv, "sampler_y");
        sampler_u = GLES20.glGetUniformLocation(program_yuv, "sampler_u");
        sampler_v = GLES20.glGetUniformLocation(program_yuv, "sampler_v");
        textureId_yuv = new int[3];
        GLES20.glGenTextures(3, textureId_yuv, 0); //创建纹理

        for (int i = 0; i < 3; i++) {
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[i]);//绑定纹理
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT); //设置环绕方式 环绕（超出纹理坐标范围） s==x t==y repeat重复
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            //过滤（纹理像素映射到坐标点）：（缩小，放大：GL_LINEAR线性）
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        }
    }

    private void initRenderMediacodec() {
        String vertexSource = MyShaderUtil.readRawTxt(context, R.raw.vertex_shader);
        String fragmentSource = MyShaderUtil.readRawTxt(context, R.raw.fragment_mediacodec);
        program_mediacodec = MyShaderUtil.createProgram(vertexSource, fragmentSource);
        avPosition_mediacodec = GLES20.glGetAttribLocation(program_mediacodec, "av_Position"); //8 得到着色器中的属性 顶点坐标
        afPosition_mediacodec = GLES20.glGetAttribLocation(program_mediacodec, "af_Position"); //纹理坐标

        samplerOES_mediacodec = GLES20.glGetUniformLocation(program_mediacodec, "sTexture");

        int[] textureids = new int[1];
        GLES20.glGenTextures(1, textureids, 0);
        textureId_mediacodec = textureids[0];
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT); //设置环绕方式 环绕（超出纹理坐标范围） s==x t==y repeat重复
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        //过滤（纹理像素映射到坐标点）：（缩小，放大：GL_LINEAR线性）
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

        surfaceTexture = new SurfaceTexture(textureId_mediacodec);
        surface = new Surface(surfaceTexture);
        surfaceTexture.setOnFrameAvailableListener(this);

        if (onSurfaceCreateListener != null) {
            onSurfaceCreateListener.onSurfaceCreate(surface);
        }
    }

    public interface OnSurfaceCreateListener {
        void onSurfaceCreate(Surface surface);
    }

    public void setYUVRenderData(int width, int height, byte[] y, byte[] u, byte[] v) {
        this.width_yuv = width;
        this.height_yuv = height;
        this.y = ByteBuffer.wrap(y);
        this.u = ByteBuffer.wrap(u);
        this.v = ByteBuffer.wrap(v);
    }

    // 渲染
    private void renderYUV() {

        if (width_yuv > 0 && height_yuv > 0 && y != null && u != null & v != null) {
            GLES20.glUseProgram(program_yuv); //9使用源程序

//        GLES20.glUniform4f(afColor, 1f, 0f, 0, 1f); //修改着色器颜色

            GLES20.glEnableVertexAttribArray(avPosition_yuv); //10使顶点属性数组有效
            GLES20.glVertexAttribPointer(avPosition_yuv, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer); //11为顶点属性赋值
//            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4); //12 绘制图形

            GLES20.glEnableVertexAttribArray(afPosition_yuv); //绘制纹理坐标
            GLES20.glVertexAttribPointer(afPosition_yuv, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[0]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width_yuv, height_yuv, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, y);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[1]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width_yuv / 2, height_yuv / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, u);

            GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId_yuv[2]);
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, width_yuv / 2, height_yuv / 2, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, v);

            GLES20.glUniform1i(sampler_y, 0);
            GLES20.glUniform1i(sampler_u, 1);
            GLES20.glUniform1i(sampler_v, 2);

            y.clear();
            u.clear();
            v.clear();
            y = null;
            u = null;
            v = null;
        }
    }
}
