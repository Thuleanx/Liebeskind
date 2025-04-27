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
layout(set = 1, binding = 5) uniform sampler2D tungstenSampler;

const uint HasTextureMap = 1u;
const uint HasNormalMap = 2u;
const uint HasDisplacementMap = 4u;
const uint HasEmissionMap = 8u;
const uint HasTungstenMap = 16u;
const uint HasAllMaps = HasTextureMap | HasNormalMap | HasDisplacementMap | HasEmissionMap;

layout(constant_id = 0) const uint samplerInclusion = HasAllMaps;

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

vec2 applyParallaxMapping(vec3 viewDirectionTangent, vec2 uv) {
    const float parallaxMappingZBias = 0.42; // arbitrary bias, prevents artifacts for shallow view angles
    const float height_scale = 0.2; // maximum penetration in the normal that the height map represents
    const int numOfLayers = 10;
    const int numOfSecondaryLayers = 10;

    float layerDepth = 1.0 / numOfLayers;

    float currentLayerDepth = 0;
    vec2 deltaUV = -viewDirectionTangent.xy * height_scale * layerDepth;
    float currentSampledDepth = 0;
    float lastSampledDepth = 0;

    int layer = 0;
    for (layer = 0; layer <= numOfLayers; layer++) {
        currentSampledDepth = texture(displacementSampler, uv).r;

        if (currentLayerDepth >= currentSampledDepth)
            break;

        lastSampledDepth = currentSampledDepth;
        uv += deltaUV;
        currentLayerDepth += layerDepth;
    }

    if (layer == 0) return uv;

    {
        // walk back one step
        uv = uv - deltaUV;
        currentLayerDepth -= layerDepth;
        deltaUV = deltaUV * layerDepth;

        layerDepth = layerDepth / numOfSecondaryLayers;

        for (layer = 0; layer <= numOfSecondaryLayers; layer++) {
            currentSampledDepth = texture(displacementSampler, uv).r;

            if (currentLayerDepth >= currentSampledDepth)
                break;

            lastSampledDepth = currentSampledDepth;
            uv += deltaUV;
            currentLayerDepth += layerDepth;
        }
    }

    float afterDepth = currentSampledDepth - currentLayerDepth;
    float beforeDepth = lastSampledDepth - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);

    return uv - deltaUV * weight;
}

void main() {
    vec3 eyePosition = vec3(scene.inverseView[3][0], scene.inverseView[3][1], scene.inverseView[3][2]);

    TangentSpace tangentSpace = calculateTangentSpace();
    // by convention, this vector points outwards from the surface
    vec3 viewDirection = normalize(eyePosition - inPositionWorld);
    vec3 viewDirectionTangent = tangentSpace.worldToTangent * viewDirection;

    vec2 parallaxUV = applyParallaxMapping(viewDirectionTangent, inFragTexCoord);
    bool isOutOfTexture = parallaxUV.x < 0 || parallaxUV.x > 1 || parallaxUV.y < 0 || parallaxUV.y > 1;
    if (isOutOfTexture)
        discard;

    vec4 texColor = texture(texSampler, parallaxUV) * vec4(inFragColor, 1.0);

    // Who allowed these to not be normalized
    vec3 sampledNormalTangent = 2.0 * texture(normalSampler, parallaxUV).xyz - 1.0;
    vec3 sampledNormalWorld = normalize(tangentSpace.tangentToWorld * sampledNormalTangent);

    // by convention, these vectors points outwards from the surface
    vec3 lightDirection = -normalize(scene.mainLightDirection);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    vec3 reflectedDirection = 2 * (dot(sampledNormalWorld, lightDirection)) * sampledNormalWorld - lightDirection;

    vec3 emissiveColor = texture(emissiveSampler, parallaxUV).xyz;
    
    float specular = pow(max(0, dot(viewDirection, reflectedDirection)), materialProperties.shininess);
    float diffuse = max(0, dot(sampledNormalWorld, lightDirection));


    vec3 lighting =
        scene.ambientColor * materialProperties.ambient + 
        scene.mainLightColor * diffuse * materialProperties.diffuse +
        scene.mainLightColor * specular * materialProperties.specular +
        emissiveColor * materialProperties.emission;

    outColor = vec4(lighting, 1) * texColor;
    //outColor = vec4(vec3(diffuse), 1);
}
