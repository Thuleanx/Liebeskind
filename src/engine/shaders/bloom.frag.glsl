#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorBuffer;
layout(set = 0, binding = 1) uniform BloomData {
    vec2 texelSize;
    float kawaseDistance;
} config;

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
    outColor = kawase_sample(texCoord * texelSize, kawaseDistance);
}

void upsample() {
    vec2 texCoord = gl_FragCoord.xy;
    outColor = kawase_sample(texCoord * texelSize, kawaseDistance);
}
