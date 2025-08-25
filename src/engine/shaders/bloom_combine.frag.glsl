#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D lowerMip;
layout(set = 0, binding = 1) uniform sampler2D colorBuffer;

layout(set = 1, binding = 0) uniform BloomConfig {
    vec2 swapchainExtent;
} config;

layout(constant_id = 0) const float sampleDistance = 1;
layout(constant_id = 1) const float texelScale = 1; 

vec4 kawase_sample(vec2 texCoord, vec2 texDisplacement) {
    return 
        ( texture(lowerMip, texCoord + texDisplacement * vec2(1, 1))
        + texture(lowerMip, texCoord + texDisplacement * vec2(1, -1))
        + texture(lowerMip, texCoord + texDisplacement * vec2(-1, 1))
        + texture(lowerMip, texCoord + texDisplacement * vec2(-1, -1)))
        / 4.0;
}

void main() {
    vec2 texCoord = gl_FragCoord.xy;
    vec2 texelSize = texelScale / config.swapchainExtent;

    vec2 uv = (texCoord + 0.5) * texelSize;

    vec3 source = texelFetch(colorBuffer, ivec2(texCoord), 0).xyz;
    vec3 bloom = kawase_sample(uv, sampleDistance * texelSize).xyz; 

    outColor = vec4(mix(source, bloom, 0.5f), 1);
}
