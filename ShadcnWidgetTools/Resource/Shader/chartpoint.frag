#version 440

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vOffset;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform PointUniforms
{
    mat4 mvp;
    vec4 transform;
    vec4 params;
};

void main()
{
    // 边缘 1px 内按距离过渡，颜色已预乘
    float coverage = clamp(params.x + 0.5 - length(vOffset), 0.0, 1.0);
    fragColor = vColor * coverage;
}
