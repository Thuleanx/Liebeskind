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

const uint HasTextureMap = 1u;
const uint HasNormalMap = 2u;
const uint HasDisplacementMap = 4u;
const uint HasEmissionMap = 8u;
const uint HasAllMaps = HasTextureMap | HasNormalMap | HasDisplacementMap | HasEmissionMap;

layout(constant_id = 0) const uint samplerInclusion = HasAllMaps;
layout(constant_id = 1) const uint parallaxMappingMode = 0;

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

    if (parallaxMappingMode == 0) {
        float height = texture(displacementSampler, uv).r;
        vec2 viewDirectionNormalizedByDepth = viewDirectionTangent.xy /
            (viewDirectionTangent.z + parallaxMappingZBias);
        vec2 deltaUV = -viewDirectionNormalizedByDepth * height_scale * height;
        return uv + deltaUV;
    } else {
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

        if (parallaxMappingMode == 3) {
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

        if (parallaxMappingMode == 1) {
            return uv;
        } else if (parallaxMappingMode > 1) {
            float afterDepth = currentSampledDepth - currentLayerDepth;
            float beforeDepth = lastSampledDepth - currentLayerDepth + layerDepth;

            float weight = afterDepth / (afterDepth - beforeDepth);

            return uv - deltaUV * weight;
        }
    }
}

void main() {
    vec3 eyePosition = vec3(scene.inverseView[3][0], scene.inverseView[3][1], scene.inverseView[3][2]);

    TangentSpace tangentSpace = calculateTangentSpace();

    // by convention, this vector points outwards from the surface
    vec3 viewDirection = normalize(eyePosition - inPositionWorld);
    vec3 viewDirectionTangent = tangentSpace.worldToTangent * viewDirection;

    vec2 uv = inFragTexCoord; 

#ifdef HAS_DISPLACEMENT
    uv = applyParallaxMapping(viewDirectionTangent, inFragTexCoord);

    bool isOutOfTexture = uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
    if (isOutOfTexture)
        discard;
#endif

    vec4 texColor = vec4(inFragColor, 1.0);
#ifdef HAS_ALBEDO
    texColor *= texture(texSampler, uv);
#endif

    // Who allowed these to not be normalized
    vec3 normalWorld = tangentSpace.normalWorld;

#ifdef HAS_NORMAL
    vec3 sampledNormalTangent = 2.0 * texture(normalSampler, uv).xyz - 1.0;
    normalWorld = normalize(tangentSpace.tangentToWorld * sampledNormalTangent);
#endif

    // by convention, these vectors points outwards from the surface
    vec3 lightDirection = -normalize(scene.mainLightDirection);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    vec3 reflectedDirection = 2 * (dot(normalWorld, lightDirection)) * normalWorld - lightDirection;
    float specular = pow(max(0, dot(viewDirection, reflectedDirection)), materialProperties.shininess);
    float diffuse = max(0, dot(normalWorld, lightDirection));

    vec3 lighting =
        scene.ambientColor * materialProperties.ambient + 
        scene.mainLightColor * diffuse * materialProperties.diffuse +
        scene.mainLightColor * specular * materialProperties.specular;

    vec3 color = texColor.xyz * lighting;

#ifdef HAS_EMISSION
    vec3 emissiveColor = texture(emissiveSampler, uv).xyz;
    color += emissiveColor * materialProperties.emission;
#else
    color += materialProperties.emission;
#endif

    outColor = vec4(color, 1);
}
