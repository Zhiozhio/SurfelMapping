
#version 330 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec4 normal;

uniform mat4 MVP;
uniform mat4 pose;
uniform float threshold;
uniform int colorType;
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

out vec4 vColor;

#include "color.glsl"

void main()
{
    if(position.w > threshold)
    {
        if(colorType == 1)  // draw normal
        {
            vColor = vec4(normal.xyz, 1.0);
        }
        else if(colorType == 2)  // draw color
        {
            uvec4 srgb = decodeColor(color.x);
            vec3 rgb = vec3(srgb.yzw) / 255.f;
            vColor = vec4(rgb, 1.0);
        }
        else if(colorType == 3)
        {
            uvec4 srgb = decodeColor(color.x);
            uint c = srgb.r;

            if(c == 0U) vColor = vec4((class0 / 255), 1.0);
            else if(c == 1U) vColor = vec4((class1 / 255), 1.0);
            else if(c == 2U) vColor = vec4((class2 / 255), 1.0);
            else if(c == 3U) vColor = vec4((class3 / 255), 1.0);
            else if(c == 4U) vColor = vec4((class4 / 255), 1.0);
            else if(c == 5U) vColor = vec4((class5 / 255), 1.0);
            else if(c == 6U) vColor = vec4((class6 / 255), 1.0);
            else if(c == 7U) vColor = vec4((class7 / 255), 1.0);
            else if(c == 8U) vColor = vec4((class8 / 255), 1.0);
            else if(c == 9U) vColor = vec4((class9 / 255), 1.0);
            else if(c == 10U) vColor = vec4((class10 / 255), 1.0);
            else if(c == 11U) vColor = vec4((class11 / 255), 1.0);
            else if(c == 12U) vColor = vec4((class12 / 255), 1.0);
            else if(c == 13U) vColor = vec4((class13 / 255), 1.0);
            else if(c == 14U) vColor = vec4((class14 / 255), 1.0);
            else if(c == 15U) vColor = vec4((class15 / 255), 1.0);
            else if(c == 16U) vColor = vec4((class16 / 255), 1.0);
            else if(c == 17U) vColor = vec4((class17 / 255), 1.0);
            else if(c == 18U) vColor = vec4((class18 / 255), 1.0);
            else vColor = vec4(0.0);
        }
        else  // draw mono (a little normal)
        {
            vColor = vec4((vec3(.5f, .5f, .5f) * abs(dot(normal.xyz, vec3(1.0, 1.0, 1.0)))) + vec3(0.1f, 0.1f, 0.1f), 1.0f);
        }
	    gl_Position = MVP * pose * vec4(position.xyz, 1.0);
    }
    else
    {
        gl_Position = vec4(-10, -10, 0, 1);
    }
}
