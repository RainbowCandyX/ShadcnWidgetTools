#version 440

// 顶点坐标为组件内的逻辑像素，颜色为预乘透明度的 RGBA
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 vColor;

// 逻辑像素到裁剪空间的正交投影，已包含各图形 API 的裁剪空间修正
layout(std140, binding = 0) uniform ChartUniforms
{
    mat4 mvp;
};

void main()
{
    vColor = color;
    gl_Position = mvp * vec4(position, 0.0, 1.0);
}
