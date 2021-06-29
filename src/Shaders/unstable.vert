
#version 330 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColorTime;
layout (location = 2) in vec4 vNormRad;

flat out vec4 positionConf;
flat out vec4 colorTime;
flat out vec4 normRadii;
flat out int emit;

void main()
{
    emit = 1;

    int mark = int(round(vColorTime.y));

    // Filter the surfel to be updated
    if(mark >= 0)
    {
        emit = 0;
    }

    // Filter the new surfel
    if(mark < 0)
    {
        positionConf = vPosition;
        colorTime = vColorTime;
        normRadii = vNormRad;

        colorTime.y = 0.0;
    }
}

