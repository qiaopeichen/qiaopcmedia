package com.example.myapplication;

import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import com.example.qiaopcplayer.Demo;
import com.example.qiaopcplayer.TimeInfoBean;
import com.example.qiaopcplayer.listener.OnCompleteListener;
import com.example.qiaopcplayer.listener.OnErrorListener;
import com.example.qiaopcplayer.listener.OnLoadListener;
import com.example.qiaopcplayer.listener.OnPauseResumeListener;
import com.example.qiaopcplayer.listener.OnPreparedListener;
import com.example.qiaopcplayer.listener.OnRecordTimeListener;
import com.example.qiaopcplayer.listener.OnTimeInfoListener;
import com.example.qiaopcplayer.listener.OnValumeDBListener;
import com.example.qiaopcplayer.log.MyLog;
import com.example.qiaopcplayer.muteenum.MuteEnum;
import com.example.qiaopcplayer.opengl.MyGLSurfaceView;
import com.example.qiaopcplayer.player.QiaopcPlayer;
import com.example.qiaopcplayer.util.TimeUtil;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.

    private QiaopcPlayer qiaopcPlayer;
    private TextView tvTime;
    private SeekBar seekBarSeek;
    private SeekBar seekBarVolume;
    TextView tvVolume;
    private int position = 0; //0-100 seekbar  value
    private boolean isSeekBar = false;
    private MyGLSurfaceView myGLSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tvTime = findViewById(R.id.tv_time);
        seekBarSeek = findViewById(R.id.seekbar_seek);
        seekBarVolume = findViewById(R.id.seekbar_volume);
        tvVolume = findViewById(R.id.tv_volume);
        myGLSurfaceView = findViewById(R.id.myglsurfaceview);
        qiaopcPlayer = new QiaopcPlayer();
        qiaopcPlayer.setMyGLSurfaceView(myGLSurfaceView);

        qiaopcPlayer.setVolume(70);
        qiaopcPlayer.setPitch(1.0f);
        qiaopcPlayer.setSpeed(1.0f);
        qiaopcPlayer.setMute(MuteEnum.MUTE_CENTER);
        tvVolume.setText("??????" + qiaopcPlayer.getVolumePercent() + "%");
        seekBarVolume.setProgress( qiaopcPlayer.getVolumePercent());
        qiaopcPlayer.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrerared() {
                MyLog.d("??????????????????????????????????????????");
                qiaopcPlayer.start();
            }
        });
        qiaopcPlayer.setOnLoadListener(new OnLoadListener() {
            @Override
            public void onLoad(boolean load) {
                if (load) {
                    MyLog.d("?????????");
                } else {
                    MyLog.d("?????????");
                }
            }
        });
        qiaopcPlayer.setOnPauseResumeListener(new OnPauseResumeListener() {
            @Override
            public void onPause(boolean pause) {
                if (pause) {
                    MyLog.d("?????????");
                } else {
                    MyLog.d("?????????");
                }
            }
        });
        qiaopcPlayer.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
//                MyLog.d(timeInfoBean.toString());
                Message message = Message.obtain();
                message.what = 1;
                message.obj = timeInfoBean;
                handler.sendMessage(message);
            }
        });
        qiaopcPlayer.setOnErrorListener(new OnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                MyLog.d("error code = " + code + ", msg = " + msg);
            }
        });
        qiaopcPlayer.setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete() {
                MyLog.d("????????????");
            }
        });
        qiaopcPlayer.setOnValumeDBListener(new OnValumeDBListener() {
            @Override
            public void onDBValue(int db) {
//                MyLog.d("???????????????" + db);
            }
        });
        qiaopcPlayer.setOnRecordTimeListener(new OnRecordTimeListener() {
            @Override
            public void onRecordTime(int recordTime) {
                MyLog.d("onRecordTime???" + recordTime);
            }
        });

        seekBarSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (qiaopcPlayer.getDuration() > 0 && isSeekBar) {
                    position = qiaopcPlayer.getDuration() * progress / 100;
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isSeekBar = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                qiaopcPlayer.seek(position);
                isSeekBar = false;
            }
        });

        seekBarVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                qiaopcPlayer.setVolume(progress);
                tvVolume.setText("??????" + qiaopcPlayer.getVolumePercent() + "%");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });
    }


    public void begin(View view) {
//        qiaopcPlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
        qiaopcPlayer.setSource("/storage/emulated/0/Download/??????.mp4");
//        qiaopcPlayer.setSource("/storage/emulated/0/Download/csgotop50.mkv");
//        qiaopcPlayer.setSource("/storage/emulated/0/Download/hevc????????????.mp4");
//        qiaopcPlayer.setSource("http://ngcdn001.cnr.cn/live/zgzs/index.m3u8");
        qiaopcPlayer.prepared();
    }

    public void pause(View view) {
        qiaopcPlayer.pause();
    }

    public void resume(View view) {
        qiaopcPlayer.resume();
    }

    Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 1) {
                if (!isSeekBar) {
                    TimeInfoBean timeInfoBean = (TimeInfoBean) msg.obj;
                    tvTime.setText(TimeUtil.secdsToDateFormat(timeInfoBean.getCurrentTime(), timeInfoBean.getTotalTime())
                            + "/" + TimeUtil.secdsToDateFormat(timeInfoBean.getTotalTime(), timeInfoBean.getTotalTime()));

                    if (!isSeekBar && timeInfoBean.getTotalTime() > 0) {
                        seekBarSeek.setProgress(timeInfoBean.getCurrentTime() * 100 / timeInfoBean.getTotalTime());
                    }
                }
            }
        }
    };

    public void stop(View view) {
        qiaopcPlayer.stop();
    }

    public void seek(View view) {
        qiaopcPlayer.seek(215);
    }

    public void next(View view) {
//        qiaopcPlayer.playNext("http://ngcdn001.cnr.cn/live/zgzs/index.m3u8");
        qiaopcPlayer.setSource("/storage/emulated/0/Download/??????.mp4");
    }

    public void left(View view) {
        qiaopcPlayer.setMute(MuteEnum.MUTE_LEFT);
    }

    public void right(View view) {
        qiaopcPlayer.setMute(MuteEnum.MUTE_RIGHT);
    }

    public void center(View view) {
        qiaopcPlayer.setMute(MuteEnum.MUTE_CENTER);
    }

    public void speed(View view) {
        qiaopcPlayer.setSpeed(1.5f);
        qiaopcPlayer.setPitch(1.0f);
    }

    public void pitch(View view) {
        qiaopcPlayer.setSpeed(1.0f);
        qiaopcPlayer.setPitch(1.5f);
    }

    public void speedpitch(View view) {
        qiaopcPlayer.setSpeed(1.5f);
        qiaopcPlayer.setPitch(1.5f);
    }

    public void normal(View view) {
        qiaopcPlayer.setSpeed(1.0f);
        qiaopcPlayer.setPitch(1.0f);
    }

    public void start_record(View view) {
        qiaopcPlayer.startRecord(new File("/storage/emulated/0/Download/textplayer.aac"));
    }

    public void pause_record(View view) {
        qiaopcPlayer.pauseRecord();
    }

    public void goon_record(View view) {
        qiaopcPlayer.resumeRecord();
    }

    public void stop_record(View view) {
        qiaopcPlayer.stopRecord();
    }
}
