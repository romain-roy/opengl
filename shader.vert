#version 450

in vec3 position;

in vec3 normal;

out vec3 normalOut;

uniform mat4 mvp;

void main()
{
	vec4 transform = mvp * vec4(position, 1.f);
	normalOut = normal * 0.5f + 0.5f;
    gl_Position = vec4(transform);
}
