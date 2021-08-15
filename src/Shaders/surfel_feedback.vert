

#version 330 core

layout (location = 0) in vec2 texcoord;

out vec4 vPosition;
out vec4 vColor;
out vec4 vNormRad;
out float zVal;

uniform sampler2D gSampler;
uniform sampler2D cSampler;
uniform usampler2D sSampler;
uniform vec4 cam; //cx, cy, 1/fx, 1/fy
uniform float cols;
uniform float rows;
uniform int time;
uniform float maxDepth;

#include "surfels.glsl"
#include "color.glsl"
#include "geometry.glsl"

void main()
{
    //Should be guaranteed to be in bounds
    float x = texcoord.x * cols;
    float y = texcoord.y * rows;

    vPosition = vec4(getVertex(texcoord.xy, x, y, cam, gSampler), 1);
    vColor = textureLod(cSampler, texcoord.xy, 0.0);
    uvec4 vClass = texture(sSampler, texcoord.xy);
    
    vec3 vNormLocal = getNormal(vPosition.xyz, texcoord.xy, x, y, cam, gSampler);
    vNormRad = vec4(vNormLocal, getRadius(vPosition.z, vNormLocal.z));

    if(vPosition.z > 0 && vPosition.z < maxDepth
                       && (int(x) + int(y)) % 2 == 1           // 1/2 sparse
//                       && int(x) % 2 == 0 && int(y) % 2 == 0   // 1/4 sparse
//                       && int(x) % 3 == 0 && int(y) % 3 == 0   // 1/9 sparse
//                       && int(x) % 4 == 0 && int(y) % 4 == 0   // 1/16 sparse
    )
    {
        zVal = vPosition.z;
    }
    else
    {
        zVal = 0;
    }



    vPosition.w = 0.9;


    vColor.x = encodeColor(vColor.xyz, vClass.r);
    
    vColor.y = 0;
    //Timestamp
    vColor.z = float(time);
    vColor.w = float(time);
}
