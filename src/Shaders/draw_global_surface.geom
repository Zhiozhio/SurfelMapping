/*
 * This file is part of ElasticFusion.
 *
 * Copyright (C) 2015 Imperial College London
 * 
 * The use of the code within this file and all code within files that 
 * make up the software that is ElasticFusion is permitted for 
 * non-commercial purposes only.  The full terms and conditions that 
 * apply to the code within this file are detailed within the LICENSE.txt 
 * file and at <http://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/elastic-fusion-license/> 
 * unless explicitly stated.  By downloading this file you agree to 
 * comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then 
 * please email researchcontracts.engineering@imperial.ac.uk.
 *
 */

#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float threshold;
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

in vec4 vColor[];
in vec4 vPosition[];
in vec4 vNormRad[];
in mat4 vMVP[];
in int colorType0[];
in int drawWindow0[];
in int vTime[];
in int timeDelta0[];

out vec3 vColor0;
out vec2 texcoord;
out float radius;
//flat out int unstablePoint;

#include "color.glsl"

void main() 
{
    if(colorType0[0] != -1)
    {
        if(colorType0[0] == 1)  // draw norm
        {
            vColor0 = vNormRad[0].xyz;
        }
        else if(colorType0[0] == 2)  // draw color
        {
            uvec4 srgb = decodeColor(vColor[0].x);
            vColor0 = vec3(srgb.yzw) / 255.f;
        }
        else if(colorType0[0] == 3)
        {
            uvec4 srgb = decodeColor(vColor[0].x);
            uint c = srgb.r;

            if(c == 0U) vColor0 = vec3((class0 / 255));
            else if(c == 1U) vColor0 = vec3(class1 / 255);
            else if(c == 2U) vColor0 = vec3(class2 / 255);
            else if(c == 3U) vColor0 = vec3(class3 / 255);
            else if(c == 4U) vColor0 = vec3(class4 / 255);
            else if(c == 5U) vColor0 = vec3(class5 / 255);
            else if(c == 6U) vColor0 = vec3(class6 / 255);
            else if(c == 7U) vColor0 = vec3(class7 / 255);
            else if(c == 8U) vColor0 = vec3(class8 / 255);
            else if(c == 9U) vColor0 = vec3(class9 / 255);
            else if(c == 10U) vColor0 = vec3(class10 / 255);
            else if(c == 11U) vColor0 = vec3(class11 / 255);
            else if(c == 12U) vColor0 = vec3(class12 / 255);
            else if(c == 13U) vColor0 = vec3(class13 / 255);
            else if(c == 14U) vColor0 = vec3(class14 / 255);
            else if(c == 15U) vColor0 = vec3(class15 / 255);
            else if(c == 16U) vColor0 = vec3(class16 / 255);
            else if(c == 17U) vColor0 = vec3(class17 / 255);
            else if(c == 18U) vColor0 = vec3(class18 / 255);
            else vColor0 = vec3(0.0);
        }
        else
        {
            vColor0 = (vec3(.5f, .5f, .5f) * abs(dot(vNormRad[0].xyz, vec3(1.0, 1.0, 1.0)))) + vec3(0.1f, 0.1f, 0.1f);
        }
    
        if(drawWindow0[0] == 1 && vTime[0] - vColor[0].w > timeDelta0[0])
        {
            vColor0 *= 0.25;
        }

		//unstablePoint = (vPosition[0].w <= threshold ? 1 : 0);
		
		radius = vNormRad[0].w;
        
        vec3 x = normalize(vec3((vNormRad[0].y - vNormRad[0].z), -vNormRad[0].x, vNormRad[0].x)) * vNormRad[0].w * 1.41421356;
        
        vec3 y = cross(vNormRad[0].xyz, x);

        texcoord = vec2(-1.0, -1.0);
        gl_Position = vMVP[0] * vec4(vPosition[0].xyz + x, 1.0);
        EmitVertex();

        texcoord = vec2(1.0, -1.0);
        gl_Position = vMVP[0] * vec4(vPosition[0].xyz + y, 1.0);
        EmitVertex();

        texcoord = vec2(-1.0, 1.0);
        gl_Position = vMVP[0] * vec4(vPosition[0].xyz - y, 1.0);
        EmitVertex();

        texcoord = vec2(1.0, 1.0);
        gl_Position = vMVP[0] * vec4(vPosition[0].xyz - x, 1.0);
        EmitVertex();

        EndPrimitive();
    }
}
