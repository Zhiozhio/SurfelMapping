
#version 330 core

flat in vec4 vPosition0;
flat in vec4 vColorTime0;
flat in vec4 vNormRad0;
flat in int draw;

layout(location = 0) out vec4 vPosition1;
layout(location = 1) out vec4 vColorTime1;
layout(location = 2) out vec4 vNormRad1;

void main()
{
    if (draw == 0)
        discard;

    vPosition1 = vPosition0;
    vColorTime1 = vColorTime0;
    vNormRad1 = vNormRad0;
}