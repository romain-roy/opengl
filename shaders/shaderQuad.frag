#version 450

in vec2 uvOut;

out vec4 color;

uniform sampler2D textureSampler;

void main()
{
	color = texture2D(textureSampler, uvOut);
	// color = vec4(uvOut, 0, 1);
}
