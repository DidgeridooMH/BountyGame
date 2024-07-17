#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUv;
layout (location = 3) in mat3 inTBN;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec4 outMaterial;

layout (set = 1, binding = 0) uniform sampler2D albedoTexture;
layout (set = 1, binding = 1) uniform sampler2D normalTexture;
layout (set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout (set = 1, binding = 3) uniform sampler2D aoTexture;

layout (push_constant) uniform MaterialConstant {
  layout (offset = 64)
  vec4 baseColorFactor;
  float metallicFactor;
  float roughnessFactor;
  float occlusionStrength;
  uint useBaseColorTexture;
  uint useNormalTexture;
  uint useMetallicRoughnessTexture;
  uint useOcclusionTexture;
} material;

void main()
{
  outColor = material.baseColorFactor;
  if (material.useBaseColorTexture > 0) {
    outColor = texture(albedoTexture, inUv);
  }
  if (outColor.a < 1) discard;

  outPosition = vec4(inPosition, 1.0);

  outNormal = vec4(inNormal, 1.0);
  if (material.useNormalTexture > 0) {
    vec3 normal = texture(normalTexture, inUv).xyz * 2.0 - 1.0;
    normal = normalize(inTBN * normal);
    outNormal = vec4(mix(inNormal, normal, 0.5), 1.0);
  }

  float metallic = material.metallicFactor;
  float roughness = material.roughnessFactor;
  if (material.useMetallicRoughnessTexture > 0) {
    vec4 metallicRoughness = texture(metallicRoughnessTexture, inUv);
    metallic *= metallicRoughness.b;
    roughness *= metallicRoughness.g;
  }
  float ao = 0.0f;
  if (material.useOcclusionTexture > 0) {
    ao = texture(aoTexture, inUv).r * material.occlusionStrength;
  }

  outMaterial = vec4(roughness, metallic, ao, 1.0);
}
