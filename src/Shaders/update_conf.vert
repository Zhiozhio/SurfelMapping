
#version 330 core

layout (location = 0) in float confID;
layout (location = 1) in vec4 vPosConf;

flat out vec4 vPosition0;

uniform int texDim;

void main()
{
    int vert_id = floatBitsToInt(confID);
    // get position by index
    int intY = vert_id / texDim;
    int intX = vert_id - (intY * texDim);

    float halfPixel = 0.5 * (1.0f / float(texDim));

    // get texture coordinate (only for even texDim)
    float x = float(intX - (texDim / 2)) / (texDim / 2.0) + halfPixel;
    float y = float(intY - (texDim / 2)) / (texDim / 2.0) + halfPixel;

    gl_Position = vec4(x, y, 0.0, 1.0);

    vPosition0 = vPosConf;  // already minus 1 of confidence
}
