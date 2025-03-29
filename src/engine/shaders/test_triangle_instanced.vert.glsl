#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 positionWorld;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 normalWorld;
layout(location = 4) out vec3 tangentWorld;

layout(binding = 0, set = 0) uniform GPUSceneData {
    mat4 view;
    mat4 inverseView;
    mat4 projection;
    mat4 viewProjection;
    vec3 ambientColor;
    vec3 mainLightDirection;
    vec3 mainLightColor;
} gpuScene;

struct InstanceData {
    mat4 transform;
};

layout(std140, binding = 0, set = 2) readonly buffer InstanceTransformBuffer {
    InstanceData instances[];
} objectBuffer;

void main() {
    mat4 transform = objectBuffer.instances[gl_InstanceIndex].transform;

    mat4 mvp = gpuScene.projection * gpuScene.view * transform;
    normalWorld = vec3(transpose(inverse(transform)) * vec4(inNormal, 0.0));
    tangentWorld = vec3(transform * vec4(inTangent, 0.0));
    gl_Position = mvp * vec4(inPosition, 1.0);
    positionWorld = (transform * vec4(inPosition, 1.0)).xyz;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
