package com.example.qiaopcplayer.util;

import android.media.MediaCodecList;

import java.util.HashMap;
import java.util.Map;

public class VideoSupportUtil {
    private static Map<String, String> codeMap = new HashMap<>();

    static {
        codeMap.put("h264","video/avc");
    }

    public static String findVideoCodecName(String ffcodecname) {
        if (codeMap.containsKey(ffcodecname)) {
            return codeMap.get(ffcodecname);
        }
        return "";
    }
    public static boolean isSupportCodec(String ffcodecname) {
        boolean supportvideo = false;
        int count = MediaCodecList.getCodecCount(); //获取支持的硬解码格式
        for (int i = 0; i < count; i++) {
            String[] types = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equals(findVideoCodecName(ffcodecname))) {
                    supportvideo = true;
                    break;
                }
            }
            if (supportvideo) {
                break;
            }
        }
        return supportvideo;
    }
}
