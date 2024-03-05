#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTexCoords;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec4 outMaterial;

void main()
{
	outPosition = vec4(inPosition, 1.0);
	outNormal = vec4(inNormal, 1.0);

	outColor = vec4(1.0, 1.0, 1.0, 1.0);

	float metallic = 0.5;
	float roughness = 0.5;
	float ao = 0.0;
	outMaterial = vec4(metallic, roughness, ao, 1.0);
}