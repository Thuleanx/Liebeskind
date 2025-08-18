#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorBuffer;
layout(set = 1, binding = 0) uniform struct BloomConfig {
    vec2 swapchainExtent;
} config;

layout(constant_id = 0) const float sampleDistance;
layout(constant_id = 1) const float texelScale; 

vec4 kawase_sample(vec2 texCoord, float texDisplacement, int mipLevel) {
    return 
        ( texture(colorBuffer, texCoord + texDisplacement * vec2(1, 1))
        + texture(colorBuffer, texCoord + texDisplacement * vec2(1, -1))
        + texture(colorBuffer, texCoord + texDisplacement * vec2(-1, 1))
        + texture(colorBuffer, texCoord + texDisplacement * vec2(-1, -1)))
        / 4.0;
}

void downsample() {
    vec2 texCoord = gl_FragCoord.xy;
    vec2 texelSize = texelScale / swapchainExtent;
    outColor = kawase_sample(texCoord * texelSize, sampleDistance * texelSize);
}

void upsample() {
    vec2 texCoord = gl_FragCoord.xy;
    vec2 texelSize = texelScale / swapchainExtent;
    outColor = kawase_sample(texCoord * texelSize, sampleDistance * texelSize);
}
