#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUv;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec4 outMaterial;

layout (set = 0, binding = 1) sampler2D albedo;
layout (set = 0, binding = 2) sampler2D normal;
layout (set = 0, binding = 3) sampler2D metallic;
layout (set = 0, binding = 4) sampler2D roughness;
layout (set = 0, binding = 5) sampler2D ao;

layout (push_constant) uniform Material
{
  bool hasAlbedo;
  vec4 albedoColor;

  bool hasNormal;

  bool hasMetallic;
  float metallicFactor;

  bool hasRoughness;
  float roughnessFactor;

  bool hasAo;
  float aoStrength;
} material;

void main()
{
  outPosition = vec4(inPosition, 1.0);
  outNormal = vec4(inNormal, 1.0);

  if (hasAlbedo)
  {
    outColor = texture(material.albedo, inUv);
  }
  else
  {
    outColor = albedoColor;
  }

  float metallic = material.metallicFactor;
  if (hasMetallic)
  {
    metallic *= texture(material.metallic, inUv).r;
  }

  float roughness = material.roughnessFactor;
  if (hasRoughness)
  {
    roughness *= texture(material.roughness, inUv).r;
  }

  float ao = 0.0;
  if (hasAo)
  {
    texture(material.ao, inUv).r * material.aoStrength;
  }

  outMaterial = vec4(metallic, roughness, ao, 1.0);
}
