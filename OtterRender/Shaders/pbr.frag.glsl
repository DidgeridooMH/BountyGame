#version 450 core

layout (location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput ainPosition;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput ainNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput ainColor;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput ainMaterial;
layout (input_attachment_index = 4, set = 0, binding = 4) uniform subpassInput ainShadowMap;

vec3 ACESFilm(vec3 color) {
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
}

vec3 calculateDirectLight(vec3 position, vec3 normal)
{
    const vec3 lightPosition = vec3(16.0, -16.0, 16.0);
    const vec3 lightColor = vec3(1.0, 1.0, 1.0);
    const float lightIntensity = 15.0;
    const float lightConstant = 1.0;
    const float lightLinear = 0.09;
    const float lightQuadratic = 0.032;
    
    float distance = length(lightPosition - position);
    float attenuation = 1.0 / (
        lightConstant +
        lightLinear * distance +
        lightQuadratic * (distance * distance));
    vec3 radiance = lightColor * attenuation * lightIntensity;

    vec3 lightDirection = normalize(lightPosition - position);
    float directionDifference = max(dot(normal, lightDirection), 0.0);

    return directionDifference * radiance;
}

void main()
{
    if (subpassLoad(ainShadowMap).r > 0.0)
    {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    if (subpassLoad(ainPosition).a < 1.0)
    {
        discard;
    }

    vec3 position = subpassLoad(ainPosition).rgb;
    vec3 normal = subpassLoad(ainNormal).rgb;

    vec3 diffuse = calculateDirectLight(position, normal);

    vec3 lighting = max(diffuse, 0.1) * subpassLoad(ainColor).rgb;

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