package com.example.qiaopcplayer.player;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.text.TextUtils;

import com.example.qiaopcplayer.TimeInfoBean;
import com.example.qiaopcplayer.listener.OnCompleteListener;
import com.example.qiaopcplayer.listener.OnErrorListener;
import com.example.qiaopcplayer.listener.OnLoadListener;
import com.example.qiaopcplayer.listener.OnPauseResumeListener;
import com.example.qiaopcplayer.listener.OnPcmInfoListener;
import com.example.qiaopcplayer.listener.OnPreparedListener;
import com.example.qiaopcplayer.listener.OnRecordTimeListener;
import com.example.qiaopcplayer.listener.OnTimeInfoListener;
import com.example.qiaopcplayer.listener.OnValumeDBListener;
import com.example.qiaopcplayer.log.MyLog;
import com.example.qiaopcplayer.muteenum.MuteEnum;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class QiaopcPlayer {

    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("avdevice-58");
        System.loadLibrary("avfilter-7");
        System.loadLibrary("avformat-58");
        System.loadLibrary("avutil-56");
        System.loadLibrary("postproc-55");
        System.loadLibrary("swresample-3");
        System.loadLibrary("swscale-5");
    }

    private static String source;
    private static boolean playNext = false;
    private static int duration = -1;
    private static int volumePercent = 100;
    private static float speed = 1.0f;
    private static float pitch = 1.0f;
    private static boolean initmediacodec = false;
    private static MuteEnum muteEnum = MuteEnum.MUTE_CENTER;
    private OnPreparedListener onPreparedListener;
    private OnLoadListener onLoadListener;
    private OnPauseResumeListener onPauseResumeListener;
    private OnTimeInfoListener onTimeInfoListener;
    private OnErrorListener onErrorListener;
    private OnCompleteListener onCompleteListener;
    private OnValumeDBListener onValumeDBListener;
    private OnRecordTimeListener onRecordTimeListener;
    private OnPcmInfoListener onPcmInfoListener;
    private static TimeInfoBean timeInfoBean;

    public void setSource(String source) {
        this.source = source;
    }

    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        this.onPreparedListener = onPreparedListener;
    }

    public void setOnLoadListener(OnLoadListener onLoadListener) {
        this.onLoadListener = onLoadListener;
    }

    public void setOnPauseResumeListener(OnPauseResumeListener onPauseResumeListener) {
        this.onPauseResumeListener = onPauseResumeListener;
    }

    public void setOnTimeInfoListener(OnTimeInfoListener onTimeInfoListener) {
        this.onTimeInfoListener = onTimeInfoListener;
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setOnCompleteListener(OnCompleteListener onCompleteListener) {
        this.onCompleteListener = onCompleteListener;
    }

    public void setOnValumeDBListener(OnValumeDBListener onValumeDBListener) {
        this.onValumeDBListener = onValumeDBListener;
    }

    public void setOnRecordTimeListener(OnRecordTimeListener onRecordTimeListener) {
        this.onRecordTimeListener = onRecordTimeListener;
    }

    public void setOnPcmInfoListener(OnPcmInfoListener onPcmInfoListener) {
        this.onPcmInfoListener = onPcmInfoListener;
    }

    public QiaopcPlayer() {

    }

    public void prepared() {
        if (TextUtils.isEmpty(source)) {
            MyLog.d("source not be empty");
            return;
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                n_prepared(source);
            }
        }).start();
    }

    public void onCallPrepared() {
        if (onPreparedListener != null) {
            onPreparedListener.onPrerared();
        }
    }

    public void start() {
        if (TextUtils.isEmpty(source)) {
            MyLog.d("source not be empty");
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                setVolume(volumePercent);
                setMute(muteEnum);
                setSpeed(speed);
                setPitch(pitch);
                n_start();
            }
        }).start();
    }

    public void pause() {
        n_pause();
        if(onPauseResumeListener != null) {
            onPauseResumeListener.onPause(true);
        }
    }

    public void resume() {
        n_resume();
        if(onPauseResumeListener != null) {
            onPauseResumeListener.onPause(false);
        }
    }

    public void stop() {
        timeInfoBean = null;
        duration = -1;
        stopRecord();
        new Thread(new Runnable() {
            @Override
            public void run() {
                n_stop();
            }
        }).start();
    }

    public void seek(int secds) {
        n_seek(secds);
    }

    public void playNext(String url) {
        source = url;
        playNext = true;
        stop();
    }

    public void onCallLoad(boolean load) {
        if (onLoadListener != null) {
            onLoadListener.onLoad(load);
        }
    }

    public void onCallTimeInfo(int currentTime, int totalTime) {
        if (onTimeInfoListener != null) {
            if (timeInfoBean == null) {
                timeInfoBean = new TimeInfoBean();
            }
            timeInfoBean.setCurrentTime(currentTime);
            timeInfoBean.setTotalTime(totalTime);
            onTimeInfoListener.onTimeInfo(timeInfoBean);
        }
    }

    public void onCallError(int code, String msg) {
        stop();
        if (onErrorListener != null) {
            onErrorListener.onError(code, msg);
        }
    }

    public void onCallComplete() {
        stop();
        if (onCompleteListener != null) {
            onCompleteListener.onComplete();
        }
    }

    public void onCallNext() {
        if (playNext) {
            playNext = false;
            prepared();
        }
    }

    public int getDuration() {
        if (duration < 0) {
            duration = n_duration();
        }
        return duration;
    }

    public void setVolume(int percent) {
        if (percent >= 0 && percent <= 100) {
            volumePercent = percent;
            n_volume(percent);
        }
    }

    public int getVolumePercent() {
        return volumePercent;
    }

    public void setMute(MuteEnum mute) {
        muteEnum = mute;
        n_mute(mute.getValue());
    }

    public void setPitch(float p) {
        pitch = p;
        n_pitch(pitch);
    }

    public void setSpeed(float s) {
        speed = s;
        n_speed(speed);
    }

    public void startRecord(File outfile) {
        if (!initmediacodec) {
            audioSamplerate = n_samplerate();
            if(audioSamplerate > 0) {
                initmediacodec = true;
                initMediacodec(audioSamplerate, outfile);
                n_startstoprecord(true);
                MyLog.d("开始录制");
            }
        }
    }

    public void stopRecord() {
        if (initmediacodec) {
            n_startstoprecord(false);
            releaseMediacodec();
            MyLog.d("完成录制");
        }
    }

    public void pauseRecord() {
        n_startstoprecord(false);
        MyLog.d("暂停录制");
    }

    public void resumeRecord() {
        n_startstoprecord(true);
        MyLog.d("继续录制");
    }

    public void cutAudioPlay(int start_time, int end_time, boolean showPcm) {
        if (n_cuteaudioplay(start_time, end_time, showPcm)) {
            start();
        } else {
            stop();
            onCallError(2001, "cutaudio params is wrong");
        }
    }

    public void onCallValumeDB(int db) {
        if (onValumeDBListener != null) {
            onValumeDBListener.onDBValue(db);
        }
    }

    public void onCallPcmInfo(byte[] buffer, int buffersize) {
        if (onPcmInfoListener != null) {
            onPcmInfoListener.onPcmInfo(buffer, buffersize);
        }
    }

    public void onCallPcmRate(int sampleRate) {
        if (onPcmInfoListener != null) {
            onPcmInfoListener.onPcmRate(sampleRate, 16, 2);
        }
    }

    private native void n_prepared(String source);
    private native void n_start();
    private native void n_pause();
    private native void n_resume();
    private native void n_stop();
    private native void n_seek(int secds);
    private native int n_duration();
    private native void n_volume(int percent);
    private native void n_mute(int mute);
    private native void n_pitch(float pitch);
    private native void n_speed(float speed);
    private native int n_samplerate();
    private native void n_startstoprecord(boolean start);
    private native boolean n_cuteaudioplay(int start_time, int end_time, boolean showPcm);

    //mediacodec
    private MediaFormat encoderFormat = null;
    private MediaCodec encoder = null;
    private FileOutputStream outputStream = null;
    private MediaCodec.BufferInfo info = null;
    private int perpcmsize = 0;
    private byte[] outByteBuffer = null;
    private int aacsamplerate = 4;
    private double recordTime = 0;
    private int audioSamplerate = 0;

    //关于AAC的ADTS头文件信息介绍 博客https://blog.csdn.net/jay100500/article/details/52955232
    private void initMediacodec(int samperate, File outfile) {
        try {
            aacsamplerate = getADTSsamplerate(samperate);
            encoderFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, samperate, 2);
            encoderFormat.setInteger(MediaFormat.KEY_BIT_RATE, 96000); // 设置码率
            encoderFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC); //adt编码等级
            encoderFormat.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 4096); //输入最大值
            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            info = new MediaCodec.BufferInfo();
            if (encoder == null) {
                MyLog.d("create encoder wrong");
                return;
            }
            recordTime = 0;
            encoder.configure(encoderFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            outputStream = new FileOutputStream(outfile); //编码后输出的文件就写到outfile里
            encoder.start();//接收输入队列并进行编码
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    private void encodecPcmToAAC(int size, byte[] buffer) {
        MyLog.d("buffer size is:" + size + " buffer byte:" + buffer.length);
        if (buffer != null && encoder != null) {
            recordTime += size * 1.0 / (audioSamplerate * 2 * (16 / 8));
//            MyLog.d("recordTime = " + recordTime);
            if (onRecordTimeListener != null) {
                onRecordTimeListener.onRecordTime((int) recordTime);
            }

            int inputBufferIndex = encoder.dequeueInputBuffer(0);//取出一个子项
            if (inputBufferIndex >= 0) {
                ByteBuffer byteBuffer = encoder.getInputBuffers()[inputBufferIndex]; //取出内存空间的操作地址
                byteBuffer.clear();
                //java.nio.BufferOverflowException
                //        at java.nio.DirectByteBuffer.put(DirectByteBuffer.java:264)
                // 解析出来的包大于4096，往里放会报错
                byteBuffer.put(buffer);
                encoder.queueInputBuffer(inputBufferIndex, 0, size, 0, 0);// 重新添加回队列
            }
            int index = encoder.dequeueOutputBuffer(info, 0); //取出编码之后的outbuffer，放到info里
            while (index >= 0) {
                try {
                    perpcmsize = info.size + 7; //aac加adt头部，需要多7个字节
                    outByteBuffer = new byte[perpcmsize];
                    ByteBuffer byteBuffer = encoder.getOutputBuffers()[index];
                    byteBuffer.position(info.offset);
                    byteBuffer.limit(info.offset + info.size);

                    addADTsHeader(outByteBuffer, perpcmsize, aacsamplerate); //添加头部信息

                    byteBuffer.get(outByteBuffer, 7, info.size); //byteBuffer放到outByteBuffer里，从第7个字节开始，到info.size为止。 前面7个留给头部信息
                    byteBuffer.position(info.offset); //设置一下position
                    outputStream.write(outByteBuffer, 0, perpcmsize);

                    encoder.releaseOutputBuffer(index, false); //输出/渲染
                    index = encoder.dequeueOutputBuffer(info, 0);
                    outByteBuffer = null;
                    MyLog.d("编码中");
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * 添加ADT头部信息
     * @param packet 压缩后的添加了7个字节的转成byte的内存地址
     * @param packetLen 内存地址的长度
     * @param samplerate 采样率
     */
    private void addADTsHeader(byte[] packet, int packetLen, int samplerate) {
        int profile = 2; // AAC LC
        int freqIdx = samplerate; //samplerate
        int chanCfg = 2; // CPE

        // fill in ADTS data
        packet[0] = (byte) 0xFF; // 0xFFF(12bit)这里只取了8位，所以还差4位放到下一个里面
        packet[1] = (byte) 0xF9; // 第一个t位放F
        packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
        packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
        packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
        packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
        packet[6] = (byte) 0xFC;
    }

    private int getADTSsamplerate(int samplerate) {
        int rate = 4;
        switch (samplerate)
        {
            case 96000:
                rate = 0;
                break;
            case 88200:
                rate = 1;
                break;
            case 64000:
                rate = 2;
                break;
            case 48000:
                rate = 3;
                break;
            case 44100:
                rate = 4;
                break;
            case 32000:
                rate = 5;
                break;
            case 24000:
                rate = 6;
                break;
            case 22050:
                rate = 7;
                break;
            case 16000:
                rate = 8;
                break;
            case 12000:
                rate = 9;
                break;
            case 11025:
                rate = 10;
                break;
            case 8000:
                rate = 11;
                break;
            case 7350:
                rate = 12;
                break;
        }
        return rate;
    }

    private void releaseMediacodec() {
        if (encoder == null) {
            return;
        }
        try {
            recordTime = 0;
            outputStream.close();
            outputStream = null;
            encoder.stop();
            encoder.release();
            encoder = null;
            encoderFormat = null;
            info = null;
            initmediacodec = false;
            MyLog.d("录制完成");
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                outputStream = null;
            }
        }
    }
}
