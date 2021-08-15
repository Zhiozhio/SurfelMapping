
#version 330 core

in vec2 texcoord;

out float FragColor;

uniform sampler2D dSampler;
uniform usampler2D sSampler;
uniform sampler2D mSampler;
uniform float cols;
uniform float rows;
uniform float stereoBorder;

void main()
{
    vec4 texel = texture(dSampler, texcoord.xy);
    float depth = texel.r;         // depth in current frame

    uvec4 utexel = texture(sSampler, texcoord.xy);
    uint c = utexel.r;  // class

    float mov = float(texture(mSampler, texcoord));

    if(mov > 0.1f)
    {
        FragColor = 0;
    }
    else
    {
        float xStep = 1.0 / cols;
        float yStep = 1.0 / rows;

        // define kernel size
        const int R = 0;
        const int stepSize = 5;

        int num = 0;

        for (int iy = -R; iy < R + 1; ++iy)
        {
            for (int ix = -R; ix < R + 1; ++ix)
            {
                if(iy == 0 && ix == 0)
                    continue;

                float texX = ix * stepSize * xStep + texcoord.x;
                float texY = iy * stepSize * yStep + texcoord.y;

                if(texX * cols < stereoBorder || texX > 1.0 || texY < 0.0 || texY > 1.0)
                    continue;

                uint c_k = uint(texture(sSampler, vec2(texX, texY)));

                float mov_k = float(texture(mSampler, vec2(texX, texY)));

                if(c_k == c && mov_k > 0.1f)
                {
                    num++;
                }
            }

        }

        if(num > 1)
        {
            FragColor = 0;
        }
        else
        {
            FragColor = depth;
        }

    }

}