
#version 330 core

layout (location = 0) in vec2 texcoord;

out vec4 vPosition;
out vec4 vColorTime;
out vec4 vNormRad;

uniform sampler2D vertConfSampler;
uniform sampler2D colorTimeSampler;
uniform sampler2D normRadSampler;

void main()
{
    vPosition = texture(vertConfSampler, texcoord);
    vColorTime = texture(colorTimeSampler, texcoord);
    vNormRad = texture(normRadSampler, texcoord);
}