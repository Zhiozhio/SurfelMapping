
#version 330 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColorTime;
layout (location = 2) in vec4 vNormRad;

flat out vec4 vPosition0;
flat out vec4 vColorTime0;
flat out vec4 vNormRad0;
flat out int draw;

uniform int texDim;

#include "color.glsl"

void main()
{
    draw = 1;

    int mark = floatBitsToInt(vColorTime.y);

    // Filter the fused surfel
    if(mark >= 0)
    {
        // get position by index
        int intY = mark / texDim;
        int intX = mark - (intY * texDim);

        float halfPixel = 0.5 * (1.0f / float(texDim));

        // get texture coordinate (only for even texDim)
        float x = float(intX - (texDim / 2)) / (texDim / 2.0) + halfPixel;
        float y = float(intY - (texDim / 2)) / (texDim / 2.0) + halfPixel;

        gl_Position = vec4(x, y, 0.0, 1.0);

        vPosition0 = vPosition;
        vColorTime0 = vColorTime;
        vNormRad0 = vNormRad;
    }

    // Filter the new surfel
    if(mark < 0)
    {
        draw = 0;
        gl_Position = vec4(-10.f, -10.f, 0.0, 1.0);
    }
}
