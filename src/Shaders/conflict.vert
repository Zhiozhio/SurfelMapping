
#version 330 core

layout (location = 0) in vec2 texcoord;

uniform sampler2D drSampler;
uniform isampler2D indexSampler;
uniform sampler2D vertConfSampler;

uniform vec4 cam; //cx, cy, 1/fx, 1/fy
uniform float cols;
uniform float rows;
uniform int scale;
uniform float minDepth;
uniform float maxDepth;
uniform mat4 pose;

out int conf_num;
out Conflict {
    float conf_id[9];  // 3 x 3
    vec4 posConf[9];
} conf_out;

void main()
{
    conf_num = 0;

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
    if(value <= minDepth || value >= maxDepth)
    {
        value = maxDepth;
    }


    // iterate subpixels in this pixel
    int iter_num = 0;

    for(int i = 0; i < scale; ++i)
    {
        for(int j = 0; j < scale; ++j)
        {
            // get texture coordinate
            float sub_x = texcoord.x - subpix_size_x * (scale - 1) / 2 + subpix_size_x * i;
            float sub_y = texcoord.y - subpix_size_y * (scale - 1) / 2 + subpix_size_y * j;

            int currentID = int(textureLod(indexSampler, vec2(sub_x, sub_y), 0.0));

            if(currentID > 0) // if it has a projection here
            {
                // get old vertex
                vec4 vertConf = textureLod(vertConfSampler, vec2(sub_x, sub_y), 0.0);

                if(vertConf.z * lambda - value * lambda > 0.5)  // eyesight ray depth larger than measurement // todo threshold
                {
                    vertConf = pose * vec4(vertConf.xyz, 1.0);
                    vertConf.w -= 10.0;  // todo adjust

                    conf_out.posConf[iter_num] = vertConf;
                    conf_out.conf_id[iter_num] = float(currentID);
                    conf_num++;
                }
            }

            ++iter_num;
        }
    }

}