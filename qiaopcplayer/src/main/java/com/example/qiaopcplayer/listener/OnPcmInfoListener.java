package com.example.qiaopcplayer.listener;

public interface OnPcmInfoListener {
    void onPcmInfo(byte[] buffer, int buffersize);

    void onPcmRate(int samplerate, int bit, int channels);
}
