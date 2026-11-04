#version 440

layout(location = 0) in vec2 corner;

// 0 到 1，原点在左上角
layout(location = 0) out vec2 vUv;

// size.xy：绘图层尺寸；range 见片段着色器
layout(std140, binding = 0) uniform ImageUniforms
{
    mat4 mvp;
    vec4 size;
    vec4 range;
};

void main()
{
    vUv = corner * 0.5 + 0.5;
    gl_Position = mvp * vec4(vUv * size.xy, 0.0, 1.0);
}
