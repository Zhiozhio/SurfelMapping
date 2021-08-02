

#version 330 core

in vec2 texcoord;

out float FragColor;

uniform usampler2D gSampler;
uniform float minD;
uniform float maxD;
uniform float cols;
uniform float stereoBorder;

void main()
{
    uvec4 texel = texture(gSampler, texcoord.xy);
    uint value = texel.r;

    if( texcoord.x * cols < stereoBorder )
    {
        FragColor = 0;
    }
    else
    {
        if( value > uint(minD * 1000.0f) && value < uint((maxD - 0.001) * 1000.0f) )
        {
            FragColor = float(value) / 1000.0f;
        }
        else
        {
            FragColor = 0;
        }
    }
}
