#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inBitangent;
layout (location = 4) in vec2 inUv;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;

layout (binding = 0) uniform ModelViewProjection
{
    mat4 view;
    mat4 proj;
} vp;

layout (push_constant) uniform Model
{
    mat4 model;
} model;

void main()
{
    outPosition = (model.model * vec4(inPosition, 1.0)).rgb;
    gl_Position = vp.proj * vp.view * model.model * vec4(inPosition, 1.0);

    // TODO: Move inverse model to the cpu.
    outNormal = mat3(transpose(inverse(model.model))) * normalize(inNormal);
}
