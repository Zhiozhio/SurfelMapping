
#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec4 normal;

uniform vec4 cam;   //cx, cy, fx, fy
uniform float cols;
uniform float rows;
uniform mat4 t_inv;
uniform float threshold;

out vec4 vColor;
out vec4 vPosHome;
out vec4 vNormRad;

void main()
{
    vPosHome = t_inv * vec4(position.xyz, 1.0);
    vPosHome.w = position.w;

    vColor = color;

    vNormRad = vec4(normalize(mat3(t_inv) * normal.xyz), normal.w);

}
