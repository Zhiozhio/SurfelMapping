
#version 330 core

layout(points) in;
layout(points, max_vertices = 1) out;

flat in vec4 positionConf[];
flat in vec4 colorTime[];
flat in vec4 normRadii[];
flat in int emit[];

out vec4 vPosition0;
out vec4 vColorTime0;
out vec4 vNormRad0;

void main() 
{
    if(emit[0] > 0)
    {
        vPosition0 = positionConf[0];
        vColorTime0 = colorTime[0];
        vNormRad0 = normRadii[0];

        EmitVertex();
        EndPrimitive(); 
    }
}
