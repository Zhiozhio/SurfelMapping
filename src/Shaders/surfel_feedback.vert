

#version 330 core

layout (location = 0) in vec2 texcoord;

out vec4 vPosition;
out vec4 vColor;
out vec4 vNormRad;
out float zVal;

uniform sampler2D gSampler;
uniform sampler2D cSampler;
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
    
    vec3 vNormLocal = getNormal(vPosition.xyz, texcoord.xy, x, y, cam, gSampler);
    vNormRad = vec4(vNormLocal, getRadius(vPosition.z, vNormLocal.z));
    
    if(vPosition.z <= 0 || vPosition.z >= maxDepth)
    {
	    zVal = 0;
    }
    else
    {
        zVal = vPosition.z;
    }

    float maxRadDist2 = (cols / 2) * (cols / 2) + (rows / 2) * (rows / 2);  //sqrt((width * 0.5)^2 + (height * 0.5)^2)
    
    vPosition.w = confidence(maxRadDist2, x, y, 1.0f);
    
    vColor.x = encodeColor(vColor.xyz);
    
    vColor.y = 0;
    //Timestamp
    vColor.z = float(time);
    vColor.w = float(time);
}
