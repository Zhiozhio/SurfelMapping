
#version 330 core

layout(points) in;
layout(points, max_vertices = 1) out;

in int conf_id[];
in vec4 conf_posConf[];

out float conflictId0;
out vec4 vPosition0;

void main()
{
    if(conf_id[0] > 0)
    {
        conflictId0 = intBitsToFloat(conf_id[0]);
        //conflictId0 = float(conf_id[0]);
        vPosition0 = conf_posConf[0];

        EmitVertex();
        EndPrimitive();
    }
}