
 
#version 330 core

in vec2 texcoord;

out vec4 FragColor;

uniform usampler2D gSampler;
uniform float minVal;
uniform float maxVal;

void main()
{
    uvec4 texel = texture(gSampler, texcoord.xy);
    uint value = texel.r;
    if(value > uint(minVal) && value < uint(maxVal))
    {
        float intensity = value / maxVal;
        FragColor = vec4(intensity, intensity, intensity, 1.0);
    }
    else 
        FragColor = vec4(0.0);
}
