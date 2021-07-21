
#version 330 core

in vec2 texcoord;

out float FragColor;

uniform sampler2D gSampler;
uniform float cols;
uniform float rows;
uniform float minD;
uniform float maxD;
uniform float diffThresh;

void main()
{
	vec4 texel = texture(gSampler, texcoord.xy);
	float depth = texel.r;
    
    if( depth <= minD || depth >= maxD )
    {
        FragColor = 0;
    }
    else
    {
		float xStep = 1.0 / cols;
		float yStep = 1.0 / rows;

		// define kernel size
	    const int R = 1;
	    const int D = R * 2 + 1;

		int supportNum = 0;

		for (int iy = -R; iy < R + 1; ++iy)
		{
			for (int ix = -R; ix < R + 1; ++ix)
			{
				if(iy == 0 && ix == 0)
					continue;

				float texX = ix * xStep + texcoord.x;
				float texY = iy * yStep + texcoord.y;

				if(texX < 0.0 || texX > 1.0 || texY < 0.0 || texY > 1.0)
					continue;
	            
	            texel = texture(gSampler, vec2(texX, texY));
				float depth_k = texel.r;
	            
	            if(abs(depth_k - depth) < diffThresh)
				{
					supportNum++;
				}


	        }
	    }

		if(supportNum >= 7)
		{
			FragColor = depth;
		}
		else
		{
			FragColor = 0;
		}

    }
}
