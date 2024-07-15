#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUv;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outColor;
layout (location = 3) out vec4 outMaterial;

layout (set = 1, binding = 0) uniform sampler2D albedo;
layout (set = 1, binding = 1) uniform sampler2D normal;
layout (set = 1, binding = 2) uniform sampler2D metallicRoughness;
layout (set = 1, binding = 3) uniform sampler2D ao;

void main()
{
  outPosition = vec4(inPosition, 1.0);
  outNormal = vec4(inNormal, 1.0);
  outColor = vec4(texture(albedo, inUv).rgb, 1.0);

  float metallic = texture(metallicRoughness, inUv).b;
  float roughness = texture(metallicRoughness, inUv).g;
  float ao = texture(ao, inUv).r;

  outMaterial = vec4(metallic, roughness, ao, 1.0);
}
