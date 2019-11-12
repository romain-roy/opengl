#version 450

struct Light
{
	vec3 position;
	vec3 color;
};

in vec3 normalOut;
in vec3 positionOut;
in vec2 uvOut;

out vec4 color;

uniform Light light;
uniform sampler2D textureSampler;

void main()
{
	vec3 lightDirection = normalize(light.position - positionOut);

	// Ambient component
	vec3 ambient = vec3(0.1);

	// Diffuse component
	float angle = max(0.0, dot(normalOut, lightDirection));
	vec3 diffuse = angle * light.color;

	vec3 combined = ambient + diffuse;

	// color = vec4(combined, 1.0);
	
	// color = vec4(uvOut, 1.0, 1.0);

	color = texture2D(textureSampler, uvOut);
}
