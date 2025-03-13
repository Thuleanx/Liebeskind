#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 positionWorld;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 normalWorld;

layout(binding = 0, set = 0) uniform GPUSceneData {
    mat4 view;
    mat4 inverseView;
    mat4 projection;
    mat4 viewProjection;
    vec3 ambientColor;
    vec3 mainLightDirection;
    vec3 mainLightColor;
} gpuScene;

layout(std140, binding = 0, set = 2) readonly buffer InstanceTransformBuffer {
    mat4 model[];
} instanceTransforms;

void main() {
    mat4 transform = instanceTransforms.model[gl_InstanceIndex];

    mat4 mvp = gpuScene.projection * gpuScene.view * transform;
    normalWorld = vec3(transpose(inverse(transform))*vec4(inNormal, 0.0));
    gl_Position = mvp * vec4(inPosition, 1.0);
    positionWorld = (transform * vec4(inPosition, 1.0)).xyz;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
