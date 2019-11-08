#version 450

in vec3 position;
in vec2 uv;
in vec3 normal;

out vec3 normalOut;
out vec3 positionOut;


uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	vec4 transform = projection * view * model * vec4(position, 1.f);
	positionOut =	(model * vec4(position, 1.0)).xyz;
	normalOut =		normalize((model * vec4(normal, 0.0))).xyz;
    gl_Position = vec4(transform);
}
