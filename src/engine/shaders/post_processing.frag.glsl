#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorBuffer;
layout(set = 0, binding = 1) uniform PostProcessingData {
    uint mode;
} config;

vec2 texcoord;

float luminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 tumblinRushmeier(vec3 color) {
    return color;
}

vec3 toneMap(vec3 color) {
    return tumblinRushmeier(color);
}

void main() {
    texcoord = gl_FragCoord.xy;

    vec3 color = texelFetch(colorBuffer, ivec2(texcoord), 0).xyz;

    // reinhard tone mapping
    color = toneMap(color);

    outColor = vec4(clamp(color, 0.0, 1.0), 1);
}
