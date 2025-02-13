#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec4 color;
}   vs_out;

void main()
{
    vs_out.color = aColor;
    gl_Position = projection * view * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
}