#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorBuffer;
layout(set = 0, binding = 1) uniform vec2 texelSize;
layout(set = 0, binding = 2) uniform float kawaseDistance;

void main() {
    vec2 texcoord = gl_FragCoord.xy;

    outColor = texelFetch(colorBuffer, ivec2(texcoord), 0);
    outColor += texture(colorBuffer, texCoord * texelSize + kawaseDistance * vec2(1, 1));
    outColor += texture(colorBuffer, texCoord * texelSize + kawaseDistance * vec2(1, -1));
    outColor += texture(colorBuffer, texCoord * texelSize + kawaseDistance * vec2(-1, 1));
    outColor += texture(colorBuffer, texCoord * texelSize + kawaseDistance * vec2(-1, -1));

    outColor /= 5.0;
}
