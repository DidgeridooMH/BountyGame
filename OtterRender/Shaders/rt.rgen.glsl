#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "rt.common.glsl"

layout (binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout (binding = 1, set = 0, rgba32f) uniform image2D outputImage;
layout (binding = 2, set = 0, rgba32f) uniform image2D ainPosition;

layout (location = 0) rayPayloadEXT HitPayload hitPayload;

void main()
{
  const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5f);
  const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);

  const vec4 position = imageLoad(ainPosition, ivec2(inUV));
  if (position.a < 1.0)
  {
    return;
  }

  traceRayEXT(topLevelAS,
    gl_RayFlagsNoneEXT,
    0xFF,
    0,
    0,
    0,
    position.xyz,
    0.001f,
    vec3(1.0f, -1.0f, 1.0f),
    1000000.0f,
    0
  );

  if (hitPayload.hit)
  {
    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(1.0f));
  }
  else
  {
    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(0.0f, 0.0f, 0.0f, 1.0f));
  }
}
