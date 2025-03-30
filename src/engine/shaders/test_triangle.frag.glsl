#version 450

layout(location = 0) in vec3 inPositionWorld;
layout(location = 1) in vec3 inFragColor;
layout(location = 2) in vec2 inFragTexCoord;
// These have been linearly interpolated, and not guaranteed to be 
// (1) unit vectors
// (2) orthogonal
layout(location = 3) in vec3 inNormalWorld; 
layout(location = 4) in vec3 inTangentWorld;

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
layout(set = 1, binding = 3) uniform sampler2D displacementSampler;

struct TangentSpace {
    vec3 normalWorld;
    vec3 tangentWorld;
    vec3 bitangentWorld;
    mat3 tangentToWorld;
    mat3 worldToTangent;
};

TangentSpace calculateTangentSpace() {
    TangentSpace tangentSpace;
    tangentSpace.normalWorld = normalize(inNormalWorld);
    tangentSpace.tangentWorld = normalize(inTangentWorld - dot(inTangentWorld, tangentSpace.normalWorld) * tangentSpace.normalWorld);
    tangentSpace.bitangentWorld = cross(tangentSpace.normalWorld, tangentSpace.tangentWorld);

    tangentSpace.tangentToWorld = mat3(tangentSpace.tangentWorld, tangentSpace.bitangentWorld, tangentSpace.normalWorld);
    // we can use transpose instead of inverse since the matrix is orthogonal
    tangentSpace.worldToTangent = transpose(tangentSpace.tangentToWorld);
    return tangentSpace;
}

struct ParallaxMappingResult {
    vec2 uv;
};

ParallaxMappingResult applyParallaxMapping(vec3 viewDirectionTangent, vec2 uv) {
    float height = texture(displacementSampler, uv).r;

    ParallaxMappingResult result;
    result.uv = uv - viewDirectionTangent.xy / viewDirectionTangent.z * height * 0.1;
    return result;
}

void main() {
    vec3 eyePosition = vec3(scene.inverseView[3][0], scene.inverseView[3][1], scene.inverseView[3][2]);

    TangentSpace tangentSpace = calculateTangentSpace();
    // by convention, this vector points outwards from the surface
    vec3 viewDirection = normalize(eyePosition - inPositionWorld);
    vec3 viewDirectionTangent = tangentSpace.worldToTangent * viewDirection;

    ParallaxMappingResult parallax = applyParallaxMapping(viewDirectionTangent, inFragTexCoord);

    vec4 texColor = texture(texSampler, parallax.uv) * vec4(inFragColor, 1.0);

    vec3 sampledNormalTangent = 2 * texture(normalSampler, parallax.uv).xyz - 1;
    vec3 sampledNormalWorld = normalize(tangentSpace.tangentToWorld * sampledNormalTangent);

    // by convention, these vectors points outwards from the surface
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
