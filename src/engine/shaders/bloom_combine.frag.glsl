#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(constant_id = 0) const int numOfLayers = 1; 

layout(set = 0, binding = 0) uniform sampler2D lowerMip;
layout(set = 0, binding = 1) uniform sampler2D currentMip;
layout(set = 0, binding = 2) uniform BloomCombineConfig {
    float intensity;
    float blurScale;
} combineConfig;

layout(set = 1, binding = 0) uniform BloomSharedConfig {
    vec2 baseMipSize;
} sharedConfig;

vec3 upsampleBlur(vec2 uv) {
    vec2 currentMipPixelSize = 2.0 / sharedConfig.baseMipSize;
    float weights[3] = float[](4, 2, 1);
    vec3 filterValue = vec3(0.0f);

    for (int du = -1; du <= 1; du++) {
        for (int dv = -1; dv <= 1; dv++) {
            vec3 sampleValue = texture(
                lowerMip, uv + combineConfig .blurScale * currentMipPixelSize 
                    * vec2(du, dv)).xyz;
            filterValue += sampleValue * weights[abs(du) + abs(dv)];
        }
    }

    return filterValue / 16;
}


void main() {
    vec3 lowerMipValue = upsampleBlur(uv); 
    vec3 currentMipValue = texelFetch(currentMip, ivec2(gl_FragCoord.xy), 0).xyz;

    outColor = vec4(mix(currentMipValue, lowerMipValue / numOfLayers, combineConfig.intensity), 1);
}
