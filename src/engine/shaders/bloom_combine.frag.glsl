#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D lowerMip;
layout(set = 0, binding = 1) uniform sampler2D currentMip;
layout(set = 0, binding = 2) uniform BloomCombineConfig {
    float intensity;
} combineConfig;

vec3 upsample(vec2 uv) {
    // Bilinear upsample
    return texture(lowerMip, uv).xyz;
}

void main() {
    vec3 lowerMipValue = upsample(uv); 
    vec3 currentMipValue = texelFetch(currentMip, ivec2(gl_FragCoord.xy), 0).xyz;

    outColor = vec4(mix(currentMipValue, lowerMipValue, combineConfig.intensity), 1);
}
