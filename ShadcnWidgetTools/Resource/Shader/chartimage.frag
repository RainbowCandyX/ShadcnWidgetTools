#version 440

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 fragColor;

// range：水平缩放与偏移，环形起始行与显示行数占纹理行数的比例
layout(std140, binding = 0) uniform ImageUniforms
{
    mat4 mvp;
    vec4 size;
    vec4 range;
};

layout(binding = 1) uniform sampler2D cellTexture;

void main()
{
    float u = vUv.x * range.x + range.y;
    float v = fract(range.z + vUv.y * range.w);
    fragColor = texture(cellTexture, vec2(u, v));
}
