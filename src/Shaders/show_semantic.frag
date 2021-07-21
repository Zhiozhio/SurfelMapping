
#version 330 core

in vec2 texcoord;

out vec4 FragColor;

uniform usampler2D eSampler;
uniform vec3 class0;
uniform vec3 class1;
uniform vec3 class2;
uniform vec3 class3;
uniform vec3 class4;
uniform vec3 class5;
uniform vec3 class6;
uniform vec3 class7;
uniform vec3 class8;
uniform vec3 class9;
uniform vec3 class10;
uniform vec3 class11;
uniform vec3 class12;
uniform vec3 class13;
uniform vec3 class14;
uniform vec3 class15;
uniform vec3 class16;
uniform vec3 class17;
uniform vec3 class18;

void main()
{
    uvec4 texel = texture(eSampler, texcoord.xy);

    uint c = texel.r;

    if(c == 0U) FragColor = vec4((class0 / 255), 1.0);
    else if(c == 1U) FragColor = vec4((class1 / 255), 1.0);
    else if(c == 2U) FragColor = vec4((class2 / 255), 1.0);
    else if(c == 3U) FragColor = vec4((class3 / 255), 1.0);
    else if(c == 4U) FragColor = vec4((class4 / 255), 1.0);
    else if(c == 5U) FragColor = vec4((class5 / 255), 1.0);
    else if(c == 6U) FragColor = vec4((class6 / 255), 1.0);
    else if(c == 7U) FragColor = vec4((class7 / 255), 1.0);
    else if(c == 8U) FragColor = vec4((class8 / 255), 1.0);
    else if(c == 9U) FragColor = vec4((class9 / 255), 1.0);
    else if(c == 10U) FragColor = vec4((class10 / 255), 1.0);
    else if(c == 11U) FragColor = vec4((class11 / 255), 1.0);
    else if(c == 12U) FragColor = vec4((class12 / 255), 1.0);
    else if(c == 13U) FragColor = vec4((class13 / 255), 1.0);
    else if(c == 14U) FragColor = vec4((class14 / 255), 1.0);
    else if(c == 15U) FragColor = vec4((class15 / 255), 1.0);
    else if(c == 16U) FragColor = vec4((class16 / 255), 1.0);
    else if(c == 17U) FragColor = vec4((class17 / 255), 1.0);
    else if(c == 18U) FragColor = vec4((class18 / 255), 1.0);
    else FragColor = vec4(0.0);

}