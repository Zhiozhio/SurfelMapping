

#version 330 core

in vec2 texcoord;

out uint FragColor;

uniform usampler2D gSampler;
uniform float cols;
uniform float rows;
uniform float minD;
uniform float maxD;
uniform float sigPix;
uniform float sigIntensity;

void main()
{
	uvec4 texel = texture(gSampler, texcoord.xy);
	uint value = texel.r;
    
    if( value >= uint((maxD - 0.001) * 1000.0f) || value <= uint(minD * 1000.0f) )
    {
        FragColor = 0U;
    }
    else
    {
	    int x = int(texcoord.x * cols);
	    int y = int(texcoord.y * rows);

		// define kernel size
	    const int R = 6;
	    const int D = R * 2 + 1;
	    
	    int tx = min(x - R + D, int(cols));
	    int ty = min(y - R + D, int(rows));
	
	    float sum1 = 0;
	    float sum2 = 0;
	    
	    for(int cy = max(y - R, 0); cy < ty; ++cy)
	    {
	        for(int cx = max(x - R, 0); cx < tx; ++cx)
	        {
	            float texX = float(cx) / cols;
	            float texY = float(cy) / rows;
	            
	            uint intensity = uint(texture(gSampler, vec2(texX, texY)));
	            
	            float space2 = (float(x) - float(cx)) * (float(x) - float(cx)) + (float(y) - float(cy)) * (float(y) - float(cy));
	            float color2 = (float(value) - float(intensity)) * (float(value) - float(intensity));
	
	            float weight = exp(-(space2 * sigPix + color2 * sigIntensity));
	
	            sum1 += float(intensity) * weight;
	            sum2 += weight;
	        }
	    }
	
	    FragColor = uint(round(sum1/sum2));
    }
}
