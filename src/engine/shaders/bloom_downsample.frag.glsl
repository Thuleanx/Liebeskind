#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

// Samplers
layout(set = 0, binding = 0) uniform sampler2D upperMip;

// Layer constants
layout(constant_id = 0) const float currentMipScale = 1; 
layout(constant_id = 1) const bool shouldUseKarisAverage = false; 

// Shared
layout(set = 1, binding = 0) uniform BloomSharedConfig {
    vec2 baseMipSize;
} config;

float luminance(vec4 value) {
    return dot(value, vec4(0.2126, 0.7152, 0.0722, 0.0));
}

vec4 average(vec4 a, vec4 b, vec4 c, vec4 d) {
    return (a + b + c + d) / 4;
}

vec4 karis_average(vec4 a, vec4 b, vec4 c, vec4 d) {
    float wa = 1.0 / (luminance(a) + 1.0);
    float wb = 1.0 / (luminance(b) + 1.0);
    float wc = 1.0 / (luminance(c) + 1.0);
    float wd = 1.0 / (luminance(d) + 1.0);

    float total = 1.0 / (wa + wb + wc + wd);
    return (a * wa + b * wb + c * wc + d * wd) * total;
}

vec4 kawase_sample(vec2 uv) {
    const float LOWER_TO_UPPER_MIP_SCALE = 2;
    vec2 upperMipPixelSize = currentMipScale 
        / config.baseMipSize
        / LOWER_TO_UPPER_MIP_SCALE;
    vec2 sampleDistance = upperMipPixelSize / 2;
    vec4 a = texture(upperMip, uv + sampleDistance * vec2(1, 1));
    vec4 b = texture(upperMip, uv + sampleDistance * vec2(1, -1));
    vec4 c = texture(upperMip, uv + sampleDistance * vec2(-1, 1));
    vec4 d = texture(upperMip, uv + sampleDistance * vec2(-1, -1));

    if (shouldUseKarisAverage)
        return karis_average(a, b, c, d);
    else
        return average(a, b, c, d);
}

void main() {
    outColor = kawase_sample(uv);
}
