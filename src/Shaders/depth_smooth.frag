
#version 330 core

in vec2 texcoord;

out float FragColor;

uniform sampler2D dSampler;
uniform usampler2D sSampler;
uniform float cols;
uniform float rows;
uniform float minD;
uniform float maxD;
uniform float stereoBorder;
uniform float sigPix;

void main()
{
    vec4 texel = texture(dSampler, texcoord.xy);
    float depth = texel.r;

    uvec4 utexel = texture(sSampler, texcoord.xy);
    uint c = utexel.r;  // class

    if( depth <= minD || depth >= maxD || c == 10U )  // 10 is sky
    {
        FragColor = 0;
    }
    else
    {
        float xStep = 1.0 / cols;
        float yStep = 1.0 / rows;

        float borderTex = stereoBorder / cols;

        // define kernel size
        const int R = 6;
        const int D = R * 2 + 1;

        float sum1 = 0;
        float sum2 = 0;
        int validNum = 0;

        for (int iy = -R; iy < R + 1; ++iy)
        {
            for (int ix = -R; ix < R + 1; ++ix)
            {
                float texX = ix * xStep + texcoord.x;
                float texY = iy * yStep + texcoord.y;

                if(texX < borderTex || texX > 1.0 || texY < 0.0 || texY > 1.0)
                    continue;

                texel = texture(dSampler, vec2(texX, texY));
                float depth_k = texel.r;

                utexel = texture(sSampler, vec2(texX, texY));
                uint c_k = utexel.r;

                if(depth_k <= minD || depth_k >= maxD || c != c_k)
                    continue;

                float space_diff2 = ix * ix + iy * iy;
                float weight = exp(-(space_diff2 * sigPix));

                sum1 += depth_k * weight;
                sum2 += weight;
                validNum++;
            }
        }

        if(validNum > 0)
        {
            FragColor = sum1 / sum2;
        }
        else
        {
            FragColor = 0;
        }

    }
}
