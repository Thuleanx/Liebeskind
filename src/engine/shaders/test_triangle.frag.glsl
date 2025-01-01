#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normalWorld;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform GPUSceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 ambientColor;
    vec4 mainLightDirection;
    vec4 mainLightColor;
} scene;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord) * vec4(fragColor, 1.0);

    float alignment = max(0, -dot(normalWorld, vec3(scene.mainLightDirection)));
    vec3 lighting = vec3(scene.ambientColor) + vec3(scene.mainLightColor) * alignment;

    outColor = vec4(lighting, 1) * texColor;
}
