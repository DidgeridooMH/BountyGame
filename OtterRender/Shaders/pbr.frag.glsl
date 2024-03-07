#version 450 core

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput ainPosition;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput ainNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput ainColor;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput ainMaterial;

void main()
{
    const vec3 lightPosition = vec3(4.0, -4.0, 4.0);
    const vec3 lightColor = vec3(1.0, 1.0, 1.0);
    const float lightIntensity = 1.0;
    const float lightConstant = 1.0;
    const float lightLinear = 1.0;
    const float lightQuadratic = 1.0;
    const vec3 ambientLighting = vec3(0.2, 0.2, 0.2);

    if (subpassLoad(ainPosition).a < 1.0)
    {
        discard;
    }

    vec3 position = subpassLoad(ainPosition).rgb;

    vec3 normal = subpassLoad(ainNormal).rgb;

    vec3 lightDirection = normalize(lightPosition - position);
    float directionDifference = max(dot(normal, lightDirection), 0.0);

    vec3 diffuse = directionDifference * lightColor;

    vec3 lighting = (diffuse + ambientLighting) * subpassLoad(ainColor).rgb;

	outColor = vec4(lighting, 1.0);
}