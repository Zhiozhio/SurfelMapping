
#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 TexCoords;

uniform float heightFrag;
uniform float widthFrag;

void main()
{
    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    TexCoords = vec2(1.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0 - 2 * widthFrag, 1.0, 0.0, 1.0);
    TexCoords = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, 1.0 - 2 * heightFrag, 0.0, 1.0);
    TexCoords = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = vec4(1.0 - 2 * widthFrag, 1.0 - 2 * heightFrag, 0.0, 1.0);
    TexCoords = vec2(0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}
