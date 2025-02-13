#version 330 core
out vec4 FragColor;

flat in     vec3 fColor;

void main()
{
    FragColor = vec4(fColor.xyz, 1.0f);
}