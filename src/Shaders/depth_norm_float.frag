
 
#version 330 core

in vec2 texcoord;

out vec4 FragColor;

uniform sampler2D gSampler;
uniform float minVal;
uniform float maxVal;

void main()
{
    vec4 texel = texture(gSampler, texcoord.xy);
    float value = texel.r;

    if(value > minVal && value < maxVal)
    {
        float intensity = value / maxVal;
        FragColor = vec4(intensity, intensity, intensity, 1.0);
    }
    else 
        FragColor = vec4(0.0);
}
