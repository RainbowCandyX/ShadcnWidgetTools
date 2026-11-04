#version 440

// corner 为单位四边形顶点（-1 到 1），position 与 color 为实例数据
layout(location = 0) in vec2 corner;
layout(location = 1) in vec2 position;
layout(location = 2) in vec4 color;

layout(location = 0) out vec4 vColor;
// 相对圆心的像素偏移
layout(location = 1) out vec2 vOffset;

// transform：数据坐标到逻辑像素（scaleX, offsetX, scaleY, offsetY）；params.x：半径
layout(std140, binding = 0) uniform PointUniforms
{
    mat4 mvp;
    vec4 transform;
    vec4 params;
};

void main()
{
    vec2 center = vec2(position.x * transform.x + transform.y, position.y * transform.z + transform.w);
    // 多出 1px 留给抗锯齿
    vec2 offset = corner * (params.x + 1.0);
    vColor = color;
    vOffset = offset;
    gl_Position = mvp * vec4(center + offset, 0.0, 1.0);
}
