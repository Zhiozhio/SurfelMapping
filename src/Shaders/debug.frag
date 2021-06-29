
#version 330 core

in  vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D fboAttachment;

void main()
{
    FragColor = texture(fboAttachment, TexCoords);
    FragColor = vec4(FragColor.xyz, 1.0);
}
