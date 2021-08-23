
#version 330 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vColorTime;
layout (location = 2) in vec4 vNormRad;

uniform sampler2D drSampler;
uniform usampler2D seSampler;

uniform vec4 cam; //cx, cy, fx, fy
uniform float cols;
uniform float rows;
uniform float minDepth;
uniform float maxDepth;
uniform mat4 t_inv;
uniform float fuseThresh;
uniform float stereoBorder;
uniform int isClean;

out int conf_id;
out vec4 conf_posConf;


void main()
{
    vec4 vPosHome = t_inv * vec4(vPosition.xyz, 1.0);

    float xl = vPosHome.x / vPosHome.z;
    float yl = vPosHome.y / vPosHome.z;

    float u = cam.z * xl + cam.x;
    float v = cam.w * yl + cam.y;

    if(u < stereoBorder || u > cols || v < 0 || v > rows || vPosHome.z <= minDepth || vPosHome.z >= maxDepth)
    {
        conf_id = -10;
        conf_posConf = vec4(0);
    }
    else
    {
        // unit plane vector
        vec3 ray = vec3(xl, yl, 1);
        float lambda = sqrt(xl * xl + yl * yl + 1);  // length of ray

        vec2 texcoord = vec2(u / cols, v / rows);

        float depth = float(texture(drSampler, texcoord));
        uint semantic = uint(texture(seSampler, texcoord));

        if(semantic == 10U)  // 10U is sky
        {
            depth = maxDepth + 1.f;
        }

        if(isClean == 0 && depth == 0.f)  // if not in clean mode
        {
            depth = maxDepth + 20.f;
        }

        float x = 0;
        float y = 0;

        if(depth * lambda - vPosHome.z * lambda > fuseThresh * vPosHome.z)  // closer than the mewest measurement
        {
            // x, y is NDC coordinate
            x = (u - (cols * 0.5)) / (cols * 0.5);
            y = (v - (rows * 0.5)) / (rows * 0.5);

            conf_id = gl_VertexID;
            conf_posConf = vPosition;
            conf_posConf.w -= 1.f;
        }
        else
        {
            x = -10;
            y = -10;
            conf_id = -20;
            conf_posConf = vec4(0);
        }
    }

}