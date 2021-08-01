
#version 330 core

flat in uvec3 vBGR;
in vec2 texcoord;
flat in uint semantic;

out uvec3 FragColor;

void main()
{
    if(dot(texcoord, texcoord) > 1.0)
        discard;

    FragColor = vBGR;
    //FragSemantic = semantic;
    //FragDepth = 0U;
}
