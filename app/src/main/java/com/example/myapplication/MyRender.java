package com.example.myapplication;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MyRender implements GLSurfaceView.Renderer {

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
//            0f, 1f,
//            1f, 1f,
//            0f, 0f,
//            1f, 0f

            // 180度旋转
            1f, 0f,
            0f, 0f,
            1f, 1f,
            0f, 1f
    }; //纹理坐标

    //2 为坐标分配本地内存地址
    private FloatBuffer vertexBuffer;
    private FloatBuffer textureBuffer;
    private int program;
    private int avPosition;
    private int afPosition;
//    private int sTexture;
    private int textureId;
    private int afColor;

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

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        String vertexSource = MyShaderUtil.readRawTxt(context, R.raw.vertex_shader);
        String fragmentSource = MyShaderUtil.readRawTxt(context, R.raw.fragment_shader);
        program = MyShaderUtil.createProgram(vertexSource, fragmentSource);
        if (program > 0) {
            avPosition = GLES20.glGetAttribLocation(program, "av_Position"); //8 得到着色器中的属性 顶点坐标
            afPosition = GLES20.glGetAttribLocation(program, "af_Position"); //纹理坐标
//            sTexture = GLES20.glGetUniformLocation(program, "sTexture"); //纹理

            int[] textureIds = new int[1];
            GLES20.glGenTextures(1, textureIds, 0); //创建纹理
            if (textureIds[0] == 0) {
                return;
            }
            textureId = textureIds[0];
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);//绑定纹理
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT); //设置环绕方式 环绕（超出纹理坐标范围） s==x t==y repeat重复
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
            //过滤（纹理像素映射到坐标点）：（缩小，放大：GL_LINEAR线性）
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

//            BitmapFactory.Options options = new BitmapFactory.Options();
//            options.inScaled = false; //不缩放
            Bitmap bitmap = BitmapFactory.decodeResource(context.getResources(), R.drawable.text);
            if (bitmap == null) {
                return;
            }

            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0); //设置图片
            bitmap.recycle();
            bitmap = null;


//            afColor = GLES20.glGetUniformLocation(program, "af_Color");

        }

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES20.glViewport(0, 0, width, height); //设置view宽高
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT); //把颜色缓冲区清屏
        GLES20.glClearColor(0.5f, 0f, 0.8f, 1.0f); // 用颜色来清屏

        GLES20.glUseProgram(program); //9使用源程序

//        GLES20.glUniform4f(afColor, 1f, 0f, 0, 1f); //修改着色器颜色

        GLES20.glEnableVertexAttribArray(avPosition); //10使顶点属性数组有效
        GLES20.glVertexAttribPointer(avPosition, 2, GLES20.GL_FLOAT, false, 8, vertexBuffer); //11为顶点属性赋值
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4); //12 绘制图形

        GLES20.glEnableVertexAttribArray(afPosition); //绘制纹理坐标
        GLES20.glVertexAttribPointer(afPosition, 2, GLES20.GL_FLOAT, false, 8, textureBuffer);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
    }
}
