
#version 330 core

layout(points) in;
layout(points, max_vertices = 1) out;

in vec4 vPosition[];
in vec4 vColorTime[];
in vec4 vNormRad[];

out vec4 vPosition0;
out vec4 vColorTime0;
out vec4 vNormRad0;

void main()
{
    if(vPosition[0].w > 0)
    {
        vPosition0 = vPosition[0];
        vColorTime0 = vColorTime[0];
        vNormRad0 = vNormRad[0];

        vColorTime0.y = 0.0f;  // reset

        EmitVertex();
        EndPrimitive();
    }
}