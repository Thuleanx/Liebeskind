#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normalWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GPUSceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 ambientColor;
    vec4 mainLightDirection;
    vec4 mainLightColor;
} scene;

layout(set = 1, binding = 0) uniform MaterialProperties {
    vec4 color;
} materialProperties;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord) * vec4(fragColor, 1.0);

    float alignment = max(0, -dot(normalWorld, vec3(scene.mainLightDirection)));
    vec3 lighting = vec3(scene.ambientColor) + vec3(scene.mainLightColor) * alignment;

    outColor = vec4(lighting, 1) * texColor;
}
