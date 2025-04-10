#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorBuffer;

vec2 texcoord;

void main() {
    texcoord = gl_FragCoord.xy;

    vec4 inColor = texelFetch(colorBuffer, ivec2(texcoord), 0);

    outColor = vec4(inColor.rrr, 1);
}
