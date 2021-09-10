package com.example.myapplication;

import android.content.Context;
import android.opengl.GLES20;
import android.util.Log;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

public class MyShaderUtil {
    public static String readRawTxt(Context context, int rawId) {
        InputStream inputStream = context.getResources().openRawResource(rawId);
        BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
        StringBuffer sb = new StringBuffer();
        try {
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append("\n");
            }
            reader.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return sb.toString();
    }

    public static int loadShader(int shaderType, String source) {
        int shader = GLES20.glCreateShader(shaderType); //1创建shader 着色器：顶点或片元
        if (shader != 0) {
            GLES20.glShaderSource(shader, source); //2加载shader源码并编译shader
            GLES20.glCompileShader(shader); //2加载shader源码并编译shader
            int[] compile = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compile, 0); //3检查是否编译成功
            if (compile[0] != GLES20.GL_TRUE) { //失败了
                Log.d("qiaopc", "shader compile error");
                GLES20.glDeleteShader(shader);
                shader = 0;
            }
        }
        return shader;
    }

    public static int createProgram(String vertexSource, String fragmentSource) {
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
        if (vertexShader == 0) {
            return 0;
        }
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
        if (fragmentShader == 0) {
            return 0;
        }
        int program = GLES20.glCreateProgram(); //4创建一个渲染程序
        if (program != 0) {
            GLES20.glAttachShader(program, vertexShader); //5将着色器程序添加到渲染程序中
            GLES20.glAttachShader(program, fragmentShader);
            GLES20.glLinkProgram(program); //6链接源程序
            int[] lineStatus = new int[1];
            GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, lineStatus, 0); //7 检查链接源程序是否成功
            if (lineStatus[0] != GLES20.GL_TRUE) {
                Log.d("qiaopc", "link program error");
                GLES20.glDeleteProgram(program);
                program = 0;
            }
        }
        return program;
    }
}
