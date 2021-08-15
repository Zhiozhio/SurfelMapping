
#version 330 core

in vec2 texcoord;

out float FragColor;

uniform sampler2D dSampler;
uniform usampler2D sSampler;
uniform sampler2D lSampler;
uniform float cols;
uniform float rows;
uniform vec4 cam;   //cx, cy, fx, fy
uniform float minD;
uniform float maxD;
uniform mat4 t_c2l;
uniform float stereoBorder;
uniform float moveThresh;


vec3 reproject(vec2 xy, float d, vec4 cam, mat4 T_c2t)  // in pixel coordinate
{
    vec3 vertex_curr = vec3((xy.x - cam.x) * d / cam.z, (xy.y - cam.y) * d / cam.w, d);
    vec4 vertex_target = T_c2t * vec4(vertex_curr, 1.0f);
    return vec3(cam.z * vertex_target.x / vertex_target.z + cam.x,
                cam.w * vertex_target.y / vertex_target.z + cam.y,
                vertex_target.z);
}

void main()
{
    vec4 texel = texture(dSampler, texcoord.xy);
    float depth = texel.r;         // depth in current frame

    uvec4 utexel = texture(sSampler, texcoord.xy);
    uint c = utexel.r;  // class

    if(texcoord.x * cols < stereoBorder || depth <= minD)
    {
        FragColor = depth;
    }
    else
    {
        // select movable things
        if(c == 13U || c == 14U || c == 15U    // car,   truck,      bus
        || c == 16U || c == 17U || c == 18U    // train, motorcycle, bicycle
        )
        {
            vec2 uv = vec2(texcoord.x * cols, texcoord.y * rows);

            vec3 uvd_last = reproject(uv, depth, cam, t_c2l);

            if( uvd_last.z <= minD || uvd_last.z >= maxD || uvd_last.x < stereoBorder || uvd_last.x > cols || uvd_last.y < 0 || uvd_last.y > rows )
            {
                FragColor = depth;   // keep the value, we don't remove this
            }
            else
            {
                vec2 texcoord_last = vec2(uvd_last.x / cols, uvd_last.y / rows);
                float depth_last = float(texture(lSampler, texcoord_last));       // depth in last frame

                float depth_hat = uvd_last.z;

                if(abs(depth_hat - depth_last) > moveThresh)
                {
                    FragColor = 0;   // remove this
                }
                else
                {
                    FragColor = depth; // keep the value
                }

            }

        }
        else
        {
            FragColor = depth; // keep the value
        }
    }

}
