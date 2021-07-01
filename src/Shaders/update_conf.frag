
#version 330 core

flat in vec4 vPosition0;

layout(location = 0) out vec4 vPosition1;

void main()
{
    vPosition1 = vPosition0;
}