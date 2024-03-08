#version 450 core

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput ainPosition;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput ainNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput ainColor;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput ainMaterial;

vec3 ACESFilm(vec3 color) {
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
}

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

    // Tonemap lighting
    // TODO: This should be a separate step in post.
    const float exposure = 1.0;
    lighting = ACESFilm(lighting * exposure);

    const float MaxCLL = 1000.0;
    lighting = clamp(lighting, 0.0, MaxCLL);

    lighting = lighting / MaxCLL * 1023;

    // Correct for gamma
    const float gamma = 1.2;
    lighting = pow(lighting, vec3(1.0 / gamma));

	outColor = vec4(lighting, 1.0);
}