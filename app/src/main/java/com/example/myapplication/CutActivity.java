package com.example.myapplication;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import com.example.qiaopcplayer.TimeInfoBean;
import com.example.qiaopcplayer.listener.OnPcmInfoListener;
import com.example.qiaopcplayer.listener.OnPreparedListener;
import com.example.qiaopcplayer.listener.OnTimeInfoListener;
import com.example.qiaopcplayer.log.MyLog;
import com.example.qiaopcplayer.player.QiaopcPlayer;

public class CutActivity extends AppCompatActivity {

    private QiaopcPlayer qiaopcPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cutaudio);
        qiaopcPlayer = new QiaopcPlayer();
        qiaopcPlayer.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrerared() {
                qiaopcPlayer.cutAudioPlay(20, 40, true);
            }
        });
        qiaopcPlayer.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
                MyLog.d(timeInfoBean.toString());
            }
        });
        qiaopcPlayer.setOnPcmInfoListener(new OnPcmInfoListener() {
            @Override
            public void onPcmInfo(byte[] buffer, int buffersize) {
                MyLog.d("buffersize:" + buffersize);
            }

            @Override
            public void onPcmRate(int samplerate, int bit, int channels) {
                MyLog.d("samplerate:" + samplerate);
            }
        });
    }

    public void cutaudio(View view) {
        qiaopcPlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
        qiaopcPlayer.prepared();
    }
}
