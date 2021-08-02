
#version 330 core

flat in uvec3 vBGR;
in vec2 texcoord;
flat in uint semantic;

layout(location = 0) out uvec3 FragColor;
layout(location = 1) out uint FragSemantic;
//layout(location = 2) out uint FragDepth;

void main()
{
    if(dot(texcoord, texcoord) > 1.0)
        discard;

    FragColor = vBGR;
    FragSemantic = semantic;
    //FragDepth = 0U;
}
