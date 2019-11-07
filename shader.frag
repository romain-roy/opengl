#version 450

in vec3 normalOut;

out vec4 color;

void main()
{
    color = vec4(abs(normalOut), 1.f);
}
