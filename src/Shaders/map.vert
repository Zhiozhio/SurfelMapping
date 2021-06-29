
#version 330 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColorTime;
layout (location = 2) in vec4 vNormRad;

flat out vec4 vPosition0;
flat out vec4 vColorTime0;
flat out vec4 vNormRad0;

uniform int texDim;

void main()
{
    int vertexId = gl_VertexID;

    // get position by index (row major)
    int intY = vertexId / texDim;
    int intX = vertexId - (intY * texDim);

    float halfPixel = 0.5 * (1.0f / float(texDim));

    // get texture coordinate (only for even texDim)
    float x = float(intX - (texDim / 2)) / (texDim / 2.0) + halfPixel;
    float y = float(intY - (texDim / 2)) / (texDim / 2.0) + halfPixel;

    gl_Position = vec4(x, y, 0.0, 1.0);

    vPosition0 = vPosition;
    vColorTime0 = vColorTime;
    vColorTime0.y = float(vertexId);
    vNormRad0 = vNormRad;
}
