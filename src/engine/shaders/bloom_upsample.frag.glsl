#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

// Samplers
layout(set = 0, binding = 0) uniform sampler2D lowerMip;
layout(set = 0, binding = 1) uniform sampler2D currentMip;

// Layer constants
layout(constant_id = 0) const float currentMipScale = 1; 

// Shared
layout(set = 1, binding = 0) uniform BloomSharedConfig {
    vec2 baseMipSize;
} sharedConfig;

// Layer uniform
layout(set = 0, binding = 2) uniform BloomLayerConfig {
    float blurScale;
} layerConfig;

vec3 blur(vec2 uv) {
    vec2 currentMipPixelSize = currentMipScale
        / sharedConfig.baseMipSize;
    float weights[3] = float[](4, 2, 1);
    vec3 filterValue = vec3(0.0f);

    for (int du = -1; du <= 1; du++) {
        for (int dv = -1; dv <= 1; dv++) {
            vec3 sampleValue = texture(
                currentMip, uv + layerConfig.blurScale * currentMipPixelSize 
                    * vec2(du, dv)).xyz;
            filterValue += sampleValue * weights[abs(du) + abs(dv)];
        }
    }

    return filterValue / 16;
}

vec3 upsample(vec2 uv) {
    // Bilinear upsample
    return texture(lowerMip, uv).xyz;
}

void main() {
    vec3 lowerMipValue = upsample(uv);
    vec3 currentMipValue = blur(uv);

    outColor = vec4(lowerMipValue + currentMipValue, 1);
}
