#version 450

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 uv;

out vec2 uvOut;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 1.f);
	uvOut = uv;
}
