#version 450

layout(location = 0) in vec3 positionWorld;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
// These have been linearly interpolated, and not guaranteed to be 
// (1) unit vectors
// (2) orthogonal
layout(location = 3) in vec3 normalWorld_interpolated; 
layout(location = 4) in vec3 tangentWorld_interpolated;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GPUSceneData {
    mat4 view;
    mat4 inverseView;
    mat4 projection;
    mat4 viewProjection;
    vec3 ambientColor;
    vec3 mainLightDirection;
    vec3 mainLightColor;
} scene;

layout(set = 1, binding = 0) uniform MaterialProperties {
    vec3 specular;
    vec3 diffuse;
    vec3 ambient;
    float shininess;
} materialProperties;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;

vec3 normalWorld;
vec3 tangentWorld;
vec3 bitangentWorld;
mat3 tangentToWorld;
mat3 worldToTangent;

void calculateTangentSpace() {
    normalWorld = normalize(normalWorld_interpolated);
    tangentWorld = normalize(tangentWorld_interpolated - dot(tangentWorld_interpolated, normalWorld) * normalWorld);
    bitangentWorld = cross(normalWorld, tangentWorld);

    tangentToWorld = mat3(tangentWorld, bitangentWorld, normalWorld);
    // we can use transpose instead of inverse since the matrix is orthogonal
    worldToTangent = transpose(tangentToWorld);
}

void main() {
    calculateTangentSpace();

    vec4 texColor = texture(texSampler, fragTexCoord) * vec4(fragColor, 1.0);
    vec3 sampledNormalTangent = 2 * texture(normalSampler, fragTexCoord).xyz - 1;
    vec3 sampledNormalWorld = normalize(tangentToWorld * sampledNormalTangent);

    vec3 eyePosition = vec3(scene.inverseView[3][0], scene.inverseView[3][1], scene.inverseView[3][2]);

    // by convention, these vectors points outwards from the surface
    vec3 viewDirection = normalize(eyePosition - positionWorld);
    vec3 lightDirection = -scene.mainLightDirection;
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    
    float specular = pow(max(0, dot(sampledNormalWorld, halfwayDirection)), materialProperties.shininess);
    float diffuse = max(0, dot(sampledNormalWorld, lightDirection));

    vec3 lighting =
        scene.ambientColor * materialProperties.ambient + 
        scene.mainLightColor * diffuse * materialProperties.diffuse +
        scene.mainLightColor * specular * materialProperties.specular;

    outColor = vec4(lighting, 1) * texColor;
}
