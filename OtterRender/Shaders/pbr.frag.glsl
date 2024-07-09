#version 450 core

layout (location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput ainPosition;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput ainNormal;
layout (input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput ainColor;
layout (input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput ainMaterial;

layout (set = 0, binding = 5) uniform CameraBuffer {
  vec3 cameraPositionWorldSpace;
};

const float PI = 3.14159265359;

vec3 ACESFilm(vec3 color) {
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
}

vec3 tonemap(vec3 color)
{
  const float exposure = 1.0;
  color = ACESFilm(color * exposure);
  return color;
}

vec3 perceptual_quantization(vec3 value)
{
  const float range = 4096.0;
  const float m1 = (2610.0 / range) * (1/4.0);
  const float m2 = (2523.0 / range) * 128.0;
  const float c1 = (3424.0 / range);
  const float c2 = (2413.0 / range) * 32.0;
  const float c3 = (2392.0 / range) * 32.0;

  value = pow(value, vec3(1.0 / m2));
  value = 10000.0 * pow(max(value - c1, 0.0) / (c2 - c3 * value), vec3(1.0 / m1));
  return value;
}

vec3 gamma_adjust(vec3 color, float gamma)
{
return pow(color, vec3(1.0 / gamma));
}

float ggxNormalDistribution(vec3 normal, vec3 halfway, float roughness)
{
  float roughnessSquared = roughness * roughness;
  float incidentContribution = max(dot(normal, halfway), 0.0);
  float incidentContributionSquared = incidentContribution * incidentContribution;

  float denominator = incidentContributionSquared * (roughnessSquared - 1.0) + 1.0;
  return roughnessSquared / (PI * denominator * denominator);
}

float schlickGGX(float cosTheta, float roughness)
{
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return cosTheta / (cosTheta * (1.0 - k) + k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

const vec3 lightPosition = vec3(16.0, -16.0, 16.0);
const vec3 lightColor = vec3(5.0);
const float lightConstant = 1.0;
const float lightLinear = 0.09;
const float lightQuadratic = 0.032;

// TODO: Grab from M-buffer.
const float roughness = 1.0;
const float metallic = 0.0;
vec3 calculateDirectLight(vec3 position, vec3 normal)
{
  float distance = length(lightPosition - position);
  float attenuation = 1.0 / (
      lightConstant +
      lightLinear * distance +
      lightQuadratic * (distance * distance));
  return lightColor * attenuation * max(dot(normal, lightPosition), 0.0);
}

void main()
{
  if (subpassLoad(ainPosition).a < 1.0)
  {
    // TODO: Draw a skybox
    discard;
  }

  //vec3 albedo = subpassLoad(ainColor).rgb;
  vec3 albedo = vec3(1.0);
  vec3 position = subpassLoad(ainPosition).rgb;
  vec3 normal = subpassLoad(ainNormal).rgb;

  vec3 radiance = calculateDirectLight(position, normal);

  vec3 lightDirection = normalize(lightPosition - position);
  vec3 viewDirection = normalize(cameraPositionWorldSpace - position);
  vec3 halfway = normalize(lightDirection + viewDirection);

  float cosTheta = max(dot(normal, viewDirection), 0.0);
  float cosThetaLight = max(dot(normal, lightDirection), 0.0);

  float ndf = ggxNormalDistribution(normal, halfway, roughness);
  float microGeometry = schlickGGX(cosTheta, roughness) *
    schlickGGX(cosThetaLight, roughness);
  vec3 F0 = mix(vec3(0.04), albedo, metallic);
  vec3 kS = fresnelSchlick(cosTheta, vec3(0.04));
  vec3 specular = (kS * ndf * microGeometry) / ((4.0 * cosTheta * cosThetaLight) + 0.0001);

  vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

  vec3 lighting = (kD * albedo / PI + specular) * radiance * cosThetaLight;
  // TODO: Include AO
  lighting += vec3(0.03) * albedo;
  lighting = tonemap(lighting);
  //lighting = perceptual_quantization(lighting);
  //lighting = gamma_adjust(lighting, 1.2);

  outColor = vec4(lighting, 1.0);
}
