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
    vec3 emission;
    float shininess;
} materialProperties;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D displacementSampler;
layout(set = 1, binding = 4) uniform sampler2D emissiveSampler;

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
    const float parallaxMappingZBias = 0.42; // arbitrary bias, prevents artifacts for shallow view angles
    const float height_scale = 0.0; // maximum penetration in the normal that the height map represents
    const int numOfLayers = 10;

    float layerDepth = 1.0 / numOfLayers;

    float currentLayerDepth = 0;
    vec2 deltaUV = -viewDirectionTangent.xy / (viewDirectionTangent.z + parallaxMappingZBias) * height_scale * layerDepth;
    float currentSampledDepth = 0;
    float lastSampledDepth = 0;

    for (int i = 0; i <= numOfLayers; i++) {
        currentSampledDepth = texture(displacementSampler, uv).r;

        if (currentLayerDepth >= currentSampledDepth)
            break;

        lastSampledDepth = currentSampledDepth;
        uv += deltaUV;
        currentLayerDepth += layerDepth;
    }

    float afterDepth = currentSampledDepth - currentLayerDepth;
    float beforeDepth = lastSampledDepth - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);

    ParallaxMappingResult result;
    result.uv = uv - deltaUV * weight;
    return result;
}

void main() {
    vec3 eyePosition = vec3(scene.inverseView[3][0], scene.inverseView[3][1], scene.inverseView[3][2]);

    TangentSpace tangentSpace = calculateTangentSpace();
    // by convention, this vector points outwards from the surface
    vec3 viewDirection = normalize(eyePosition - inPositionWorld);
    vec3 viewDirectionTangent = tangentSpace.worldToTangent * viewDirection;

    ParallaxMappingResult parallax = applyParallaxMapping(viewDirectionTangent, inFragTexCoord);
    bool isOutOfTexture = parallax.uv.x < 0 || parallax.uv.x > 1 || parallax.uv.y < 0 || parallax.uv.y > 1;
    if (isOutOfTexture)
        discard;

    vec4 texColor = texture(texSampler, parallax.uv) * vec4(inFragColor, 1.0);

    // Who allowed these to not be normalized
    vec3 sampledNormalTangent = 2.0 * texture(normalSampler, parallax.uv).xyz - 1.0;
    vec3 sampledNormalWorld = normalize(tangentSpace.tangentToWorld * sampledNormalTangent);

    // by convention, these vectors points outwards from the surface
    vec3 lightDirection = -scene.mainLightDirection;
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    vec3 emissiveColor = texture(emissiveSampler, parallax.uv).xyz;
    
    float specular = pow(max(0, dot(sampledNormalWorld, halfwayDirection)), materialProperties.shininess);
    float diffuse = max(0, dot(sampledNormalWorld, lightDirection));

    vec3 lighting =
        scene.ambientColor * materialProperties.ambient + 
        scene.mainLightColor * diffuse * materialProperties.diffuse +
        scene.mainLightColor * specular * materialProperties.specular +
        emissiveColor * materialProperties.emission;

    outColor = vec4(lighting, 1) * texColor;
}
