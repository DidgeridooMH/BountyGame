#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable

#include "rt.common.glsl"

layout (binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout (binding = 1, set = 0, rgba32f) uniform image2D outputImage;
layout (binding = 2, set = 0, rgba32f) uniform image2D ainPosition;
layout (binding = 3, set = 0, rgba32f) uniform image2D ainNormal;

layout (location = 0) rayPayloadEXT HitPayload hitPayload;

void main()
{
  ivec2 outputImageSize = imageSize(outputImage);
  if (gl_LaunchIDEXT.x >= outputImageSize.x || gl_LaunchIDEXT.y >= outputImageSize.y)
  {
    return;
  }

  const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5f);
  const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);

  float tmin = 0.001f;
  float tmax = 10000.0f;
  const vec4 position = imageLoad(ainPosition, ivec2(gl_LaunchIDEXT.xy));
  if (position.a < 1.0)
  {
    tmin = 0;
    tmax = 0;
  }

  const vec4 normal = imageLoad(ainNormal, ivec2(gl_LaunchIDEXT.xy));

  // TODO: Right now we're using tmin to fix the self-intersection issue. This is not a good solution.
  // We should instead use a proper epsilon value to avoid self-intersection.
  // This will also include calculating a proper normal to use for offsetting.
  traceRayEXT(topLevelAS,
    gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT,
    0xFF,
    0,
    0,
    0,
    position.xyz + normal.xyz * 0.001f,
    tmin,
    normalize(vec3(1.0f, -1.0f, 1.0f)),
    tmax,
    0
  );

  float occlusion = 0.0f;
  if (hitPayload.distance > 0.0f)
  {
    occlusion = 1.0f;
  }
  imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(occlusion, 0.0, 0.0, 1.0));
}
