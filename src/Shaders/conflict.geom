
#version 330 core

layout(points) in;
layout(points, max_vertices = 9) out;

in int conf_num[];
in Conflict {
    float conf_id[9];
    vec4 posConf[9];
} conflict_in[];

out float conflictId0;
out vec4 vPosition0;

void main()
{
    for(int i = 0; i < conf_num[0]; ++i)
    {
        conflictId0 = conflict_in[0].conf_id[i];
        vPosition0 = conflict_in[0].posConf[i];
        EmitVertex();
        EndPrimitive();
    }
}