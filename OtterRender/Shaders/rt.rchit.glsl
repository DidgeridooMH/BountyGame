#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "rt.common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main()
{
  payload.distance = gl_HitTEXT;
}

