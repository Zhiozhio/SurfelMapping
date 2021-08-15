
#version 330 core

layout (location = 0) in vec2 texcoord;

out vec4 vPosition;
out vec4 vColor;
out vec4 vNormRad;

uniform sampler2D cSampler;
uniform sampler2D drSampler;
uniform usampler2D sSampler;
uniform isampler2D indexSampler;
uniform sampler2D vertConfSampler;
uniform sampler2D colorTimeSampler;
uniform sampler2D normRadSampler;

uniform vec4 cam; //cx, cy, 1/fx, 1/fy
uniform float cols;
uniform float rows;
uniform int scale;
uniform float texDim;
uniform mat4 pose;
uniform float minDepth;
uniform float maxDepth;
uniform float time;
uniform float fuseThresh;

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
    // Raw data pixel, should be guaranteed to be in bounds and centred on pixels
    float x = texcoord.x * cols;
    float y = texcoord.y * rows;

    // unit plane coordinates
    float xl = (x - cam.x) * cam.z;
    float yl = (y - cam.y) * cam.w;

    // unit plane vector
    vec3 ray = vec3(xl, yl, 1);
    float lambda = sqrt(xl * xl + yl * yl + 1);  // length of ray

    // pixel and sub pixel size
    float pix_size_x = 1.0f / cols;
    float pix_size_y = 1.0f / rows;

    float subpix_size_x = 1.0f / (cols * scale);
    float subpix_size_y = 1.0f / (rows * scale);

    // intensity of this pixel
    float value = float(texture(drSampler, texcoord));


    //============ Associate with Model Surfels ============//

    // If this point is actually a valid vertex
    if(checkNeighbours(texcoord.xy, drSampler) && value > minDepth && value < maxDepth
                                               && (int(x) + int(y)) % 2 == 1           // 1/2 sparse
//                                               && int(x) % 2 == 0 && int(y) % 2 == 0   // 1/4 sparse
//                                               && int(x) % 3 == 0 && int(y) % 3 == 0   // 1/9 sparse
//                                               && int(x) % 4 == 0 && int(y) % 4 == 0   // 1/16 sparse
    )
    {
        //============ Calculate new surfels locally ============//

        // position
        vec3 vPosLocal = getVertex(texcoord.xy, x, y, cam, drSampler);
        // normal
        vec3 vNormLocal = getNormal(vPosLocal, texcoord.xy, x, y, cam, drSampler);

        // confidence todo new method
        //float maxRadDist2 = (cols / 2) * (cols / 2) + (rows / 2) * (rows / 2);
        //float c_n = confidence(maxRadDist2, x, y, 1.0);
        float c_n = 0.9;

        // color
        vec4 texColor = textureLod(cSampler, texcoord.xy, 0.0);
        vec3 color_n = texColor.xyz;
        float radii_n = getRadius(vPosLocal.z, vNormLocal.z);

        // semantic
        uint sem_n = uint(texture(sSampler, texcoord.xy));


        //============ Find update | new  ============//

        int updateCounter = 0;
        int bestID = 0;
        float bestDist = 1000;
        vec3 posLocal_o;
        float c_o;
        vec4 normRadLocal_o;
        vec3 color_o;
        float initTime_o;

        int windowSize = scale + 2 * 0;  // kernel window size in subpixel

        // find in near subpixels
        for(int i = 0; i < windowSize; ++i)
        {
            for(int j = 0; j < windowSize; ++j)
            {
                // get texture coordinate
                float sub_x = texcoord.x - subpix_size_x * (windowSize - 1) / 2 + subpix_size_x * i;
                float sub_y = texcoord.y - subpix_size_y * (windowSize - 1) / 2 + subpix_size_y * j;

                if(sub_x < 0.0 || sub_x > 1.0 || sub_y < 0.0 || sub_y > 1.0)
                    continue;

                int currentID = int(textureLod(indexSampler, vec2(sub_x, sub_y), 0.0));

                if(currentID > 0) // if it has a projection here
                {
                    // get old vertex
                    vec4 vertConf = textureLod(vertConfSampler, vec2(sub_x, sub_y), 0.0);
                    // get old semantic & color
                    vec4 colorTime = textureLod(colorTimeSampler, vec2(sub_x, sub_y), 0.0);
                    uvec4 srgb = decodeColor(colorTime.x);
                    uint sem_o = srgb.x;

                    if(sem_n == sem_o && abs(vertConf.z * lambda - vPosLocal.z * lambda) < fuseThresh)  // eyesight ray depth test // todo threshold
                    {
                        float dist = length(cross(ray, vertConf.xyz)) / length(ray);  // eyesight ray distance test

                        vec4 normRad = textureLod(normRadSampler, vec2(sub_x, sub_y), 0.0);

                        // closest                                                        28 degree
                        if(dist < bestDist && abs(angleBetween(normRad.xyz, vNormLocal.xyz)) < 0.5f)  // todo consider color
                        {
                            updateCounter++;
                            bestDist = dist;
                            bestID = currentID;
                            posLocal_o = vertConf.xyz;
                            c_o = vertConf.w;
                            normRadLocal_o = normRad;
                            color_o = vec3(srgb.yzw) / 255.f;
                            initTime_o = colorTime.z;
                        }
                    }
                }
            }
        }

        if(updateCounter > 0) // We found a point to merge with
        {
            // compute the merged surfel with its original ID
            if(radii_n < 1.5 * normRadLocal_o.w)
            {
                vec3 posLocal_n = ((c_n * vPosLocal.xyz) + (c_o * posLocal_o)) / (c_n + c_o);
                vPosition = pose * vec4(posLocal_n, 1.0);
                vPosition.w = c_n + c_o;

                vec3 avgColor = ((c_n * color_n) + (c_o * color_n)) / (c_n + c_o);

                vColor.x = encodeColor(avgColor, sem_n);
                vColor.y = intBitsToFloat(bestID);              // marks as the ID of model surfel to be updated
                vColor.z = initTime_o;
                vColor.w = time;

                vNormRad = ((c_n * vec4(vNormLocal, radii_n)) + (c_o * normRadLocal_o)) / (c_n + c_o);
                vNormRad.xyz = mat3(pose) * vNormRad.xyz;
                vNormRad.xyz = normalize(vNormRad.xyz);
                vNormRad.w = radii_n > normRadLocal_o.w ? normRadLocal_o.w : radii_n;
            }
            else // still the old
            {
                vPosition = pose * vec4(posLocal_o, 1.0);
                vPosition.w = c_n + c_o;

                vColor.x = encodeColor(color_o, sem_n);
                vColor.y = intBitsToFloat(bestID);              // marks as the ID of model surfel to be updated
                vColor.z = initTime_o;
                vColor.w = time;

                vNormRad.xyz = mat3(pose) * normRadLocal_o.xyz;
                vNormRad.xyz = normalize(vNormRad.xyz);
                vNormRad.w = normRadLocal_o.w;
            }
        }
        else  // construct the new unstable surfel
        {
            // position
            vPosition = pose * vec4(vPosLocal, 1);
            vPosition.w = c_n;

            // normal & radii
            vNormRad = vec4(mat3(pose) * vNormLocal, radii_n);  // rotation * normal
            vNormRad.xyz = normalize(vNormRad.xyz);

            // color
            vColor.x = encodeColor(color_n, sem_n);
            vColor.y = -1.f;                           // marks as new unstable
            vColor.z = time;
            vColor.w = time;
        }
    }
    else  //============ Other vertex ============//
    {
        vPosition = vec4(0.0);
        vNormRad = vec4(0.0);
        vColor.y = -10.f;
    }

}
