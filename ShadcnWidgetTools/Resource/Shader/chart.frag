#version 440

layout(location = 0) in vec4 vColor;

layout(location = 0) out vec4 fragColor;

void main()
{
    // 颜色已预乘透明度，混合方式为 One / OneMinusSrcAlpha
    fragColor = vColor;
}
