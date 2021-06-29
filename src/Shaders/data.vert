
#version 330 core

layout (location = 0) in vec2 texcoord;

out vec4 vPosition;
out vec4 vColor;
out vec4 vNormRad;

flat out int emitExtra;
out vec4 extraPos;
out vec4 extraCol;
out vec4 extraNor;

uniform sampler2D cSampler;
uniform sampler2D drSampler;
uniform isampler2D indexSampler;
uniform sampler2D vertConfSampler;
uniform sampler2D colorTimeSampler;
uniform sampler2D normRadSampler;

uniform vec4 cam; //cx, cy, 1/fx, 1/fy
uniform float cols;
uniform float rows;
uniform float scale;
uniform float texDim;
uniform mat4 pose;
uniform float maxDepth;
uniform float time;

#include "surfels.glsl"
#include "color.glsl"
#include "geometry.glsl"

bool checkNeighbours(vec2 texCoord, sampler2D depth)
{
    float z = float(textureLod(depth, vec2(texCoord.x - (1.0 / cols), texCoord.y), 0.0));
    if(z == 0)
        return false;
        
    z = float(textureLod(depth, vec2(texCoord.x, texCoord.y - (1.0 / rows)), 0.0));
    if(z == 0)
        return false;

    z = float(textureLod(depth, vec2(texCoord.x + (1.0 / cols), texCoord.y), 0.0));
    if(z == 0)
        return false;
        
    z = float(textureLod(depth, vec2(texCoord.x, texCoord.y + (1.0 / rows)), 0.0));
    if(z == 0)
        return false;
        
    return true;
}

float angleBetween(vec3 a, vec3 b)
{
    return acos(dot(a, b) / (length(a) * length(b)));
}

void main()
{
    emitExtra = 0;

    // Raw data pixel, should be guaranteed to be in bounds and centred on pixels
    float x = texcoord.x * cols;
    float y = texcoord.y * rows;

    //============ Calculate new surfels locally ============//
    vec3 vPosLocal = getVertex(texcoord.xy, x, y, cam, drSampler);

    // Normal
    vec3 vNormLocal = getNormal(vPosLocal, texcoord.xy, x, y, cam, drSampler);

    //============ Find update | new | invalid ============//

    // If this point is actually a valid vertex && valid neighbours
    if(int(x) % 2 == int(time) % 2 && int(y) % 2 == int(time) % 2 && 
       checkNeighbours(texcoord.xy, drSampler) &&
       vPosLocal.z > 0 && vPosLocal.z < maxDepth)
    {
	    float indexXStep = (1.0f / (cols * scale));
	    float indexYStep = (1.0f / (rows * scale));
        float windowSize = 1;  // half window size (pixels, not subpixel)

        int counter = 0;
        int bestID = 0;
	    float bestDist = 1000;
        vec3 posLocal_o;
        float c_o;
        vec4 normRadLocal_o;
        vec3 color_o;
        float initTime_o;

        float xl = (x - cam.x) * cam.z;  // unit plane coordinates
        float yl = (y - cam.y) * cam.w;
        float lambda = sqrt(xl * xl + yl * yl + 1);
        vec3 ray = vec3(xl, yl, 1);
	    
	    for(float i = texcoord.x - (scale * indexXStep * windowSize); i < texcoord.x + (scale * indexXStep * windowSize) + indexXStep; i += indexXStep)
	    {
	        for(float j = texcoord.y - (scale * indexYStep * windowSize); j < texcoord.y + (scale * indexYStep * windowSize) + indexYStep; j += indexYStep)
	        {
	           int currentID = int(textureLod(indexSampler, vec2(i, j), 0.0));
	           
	           if(currentID > 0)
	           {
                   vec4 vertConf = textureLod(vertConfSampler, vec2(i, j), 0.0);

                   if(abs((vertConf.z * lambda) - (vPosLocal.z * lambda)) < 1.0)  // eyesight ray depth test // todo
                   {
                       float dist = length(cross(ray, vertConf.xyz)) / length(ray);  // eyesight ray distance test todo should also consider confidence
                       
                       vec4 normRad = textureLod(normRadSampler, vec2(i, j), 0.0);

                       // closest            41 degree                  28 degree
                       if(dist < bestDist && (abs(normRad.z) < 0.75f || abs(angleBetween(normRad.xyz, vNormLocal.xyz)) < 0.5f))  // todo
                       {
                           counter++;
                           bestDist = dist;
                           bestID = currentID;
                           posLocal_o = vertConf.xyz;
                           c_o = vertConf.w;
                           normRadLocal_o = normRad;
                           vec4 colorTime = textureLod(colorTimeSampler, vec2(i, j), 0.0);
                           color_o = decodeColor(colorTime.x);
                           initTime_o = colorTime.z;
                       }
                   }
	           }
	        }
	    }


        // Complete information of this new surfel
        // confidence todo
        float maxRadDist2 = (cols / 2) * (cols / 2) + (rows / 2) * (rows / 2);
        float c_n = confidence(maxRadDist2, x, y, 1.0);

        vec4 texColor = textureLod(cSampler, texcoord.xy, 0.0);
        vec3 color_n = texColor.xyz;
        float radii_n = getRadius(vPosLocal.z, vNormLocal.z);


	    // We found a point to merge with
	    if(counter > 0)
	    {
            // compute the merged surfel with its original ID
            if(radii_n < (1.0 + 0.5) * normRadLocal_o.w)
            {
                vec3 posLocal_n = ((c_n * vPosLocal.xyz) + (c_o * posLocal_o)) / (c_n + c_o);
                vPosition = pose * vec4(posLocal_n, 1.0);
                vPosition.w = c_n + c_o;

                vec3 avgColor = ((c_n * color_n) + (c_o * color_n)) / (c_n + c_o);
                vColor.x = encodeColor(avgColor);
                vColor.y = float(bestID);              // marks as the ID of model surfel to be updated
                vColor.z = initTime_o;
                vColor.w = time;

                vNormRad = ((c_n * vec4(vNormLocal, radii_n)) + (c_o * normRadLocal_o)) / (c_n + c_o);
                vNormRad.xyz = mat3(pose) * vNormRad.xyz;
                vNormRad.xyz = normalize(vNormRad.xyz);
            }
            else // still the old
            {
                vPosition = pose * vec4(posLocal_o, 1.0);
                vPosition.w = c_n + c_o;

                vColor.x = encodeColor(color_o);
                vColor.y = float(bestID);              // marks as the ID of model surfel to be updated
                vColor.z = initTime_o;
                vColor.w = time;

                vNormRad.xyz = mat3(pose) * normRadLocal_o.xyz;
                vNormRad.xyz = normalize(vNormRad.xyz);
                vNormRad.w = normRadLocal_o.w;
            }
	    }
	    else
	    {
            // construct the new unstable surfel
            // position
            vPosition = pose * vec4(vPosLocal, 1);
            vPosition.w = c_n;

            // normal & radii
            vNormRad = vec4(mat3(pose) * vNormLocal, radii_n);  // rotation * normal
            vNormRad.xyz = normalize(vNormRad.xyz);

            // color
            vColor.x = encodeColor(color_n);
            vColor.y = -1.f;                           // marks as new unstable
            vColor.z = time;
            vColor.w = time;

            // if model is invalid  todo we should consider radius
            int current_id = int(textureLod(indexSampler, texcoord, 0.0));
            vec4 vert = textureLod(vertConfSampler, texcoord.xy, 0.0);
            if(current_id > 0 && vert.z < vPosLocal.z)
            {
                emitExtra = 1;

                extraPos = pose * vec4(vert.xyz, 1.0);
                extraPos.w = vert.w - 1.1;         // decrease the confidence

                extraNor = textureLod(normRadSampler, texcoord.xy, 0.0);
                extraNor.xyz = mat3(pose) * extraNor.xyz;
                extraNor.xyz = normalize(extraNor.xyz);

                extraCol = textureLod(colorTimeSampler, texcoord.xy, 0.0);
                extraCol.y = float(current_id);          // marks as the ID of model surfel to be updated
                extraCol.w = time;
            }

	    }
    }
    else  //============ Other vertex ============//
    {
        vPosition = vec4(0.0);
        vNormRad = vec4(0.0);
        vColor.y = -10.f;
    }

}
