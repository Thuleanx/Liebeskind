#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 outPositionWS;

layout(binding = 0, set = 0) uniform GPUSceneData {
    mat4 projection;
} gpuScene;

layout(push_constant) uniform PushConstants {
    mat4 model;
} objData;

void main() {
    mat4 transform = objData.model;

    mat4 mvp = gpuScene.projection * transform;

    outPositionWS = (transform * vec4(inPosition, 1.0)).xyz;
    gl_Position = mvp * vec4(inPosition, 1.0);
}
