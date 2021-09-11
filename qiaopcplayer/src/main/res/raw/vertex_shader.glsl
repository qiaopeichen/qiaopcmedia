attribute vec4 av_Position;
attribute vec2 af_Position;
varying vec2 v_texPosition;
void main() {
    v_texPosition = af_Position;
    gl_Position = av_Position;
}

//attribute只能在vertex中使用，varying 用于vertex和fragment之间传递值