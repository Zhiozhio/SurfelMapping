
#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec4 cam;   //cx, cy, fx, fy
uniform float cols;
uniform float rows;
uniform mat4 t_inv;
uniform float maxDepth;
uniform float threshold;

in vec4 vColor[];
in vec4 vPosHome[];
in vec4 vNormRad[];

flat out uvec3 vBGR;
out vec2 texcoord;
flat out uint semantic;

#include "color.glsl"

vec3 projectPoint(vec3 p)
{
    return vec3(((((cam.z * p.x) / p.z) + cam.x) - (cols * 0.5)) / (cols * 0.5),
                ((((cam.w * p.y) / p.z) + cam.y) - (rows * 0.5)) / (rows * 0.5),
                (2 * p.z / maxDepth) - 1);
}

void main()
{
    uvec4 srgb = decodeColor(vColor[0].x);
    vBGR = srgb.wzy;
    semantic = srgb.r + 1U;            // 0 is null, every class add by 1

    if(vPosHome[0].z >= maxDepth || vPosHome[0].z <= 1.f)
    {
        gl_Position = vec4(-10, -10, 1, 0);
    }
    else
    {
        vec3 x, y;

        if(vPosHome[0].z > 5)
        {
            vec3 tmpNorm = vec3(0, 0, 1);
            x = normalize(vec3((tmpNorm.y - tmpNorm.z), -tmpNorm.x, tmpNorm.x)) * vNormRad[0].w * 1.41421356;
            y = cross(tmpNorm.xyz, x);
        }
        else
        {
            vec3 eyeToVert = vPosHome[0].xyz;
            float cosAngle = dot(eyeToVert, vNormRad[0].xyz) / (length(eyeToVert) * length(vNormRad[0].xyz));

            float radius = vNormRad[0].w / (1.0 + 0.5 * abs(cosAngle));

            x = normalize(vec3((vNormRad[0].y - vNormRad[0].z), -vNormRad[0].x, vNormRad[0].x)) * radius * 1.41421356;
            y = cross(vNormRad[0].xyz, x);
        }

        texcoord = vec2(-1.0, -1.0);
        gl_Position = vec4(projectPoint(vPosHome[0].xyz + x), 1.0);
        EmitVertex();

        texcoord = vec2(1.0, -1.0);
        gl_Position = vec4(projectPoint(vPosHome[0].xyz + y), 1.0);
        EmitVertex();

        texcoord = vec2(-1.0, 1.0);
        gl_Position = vec4(projectPoint(vPosHome[0].xyz - y), 1.0);
        EmitVertex();

        texcoord = vec2(1.0, 1.0);
        gl_Position = vec4(projectPoint(vPosHome[0].xyz - x), 1.0);
        EmitVertex();

        EndPrimitive();
    }
}
