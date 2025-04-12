#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorBuffer;

vec2 texcoord;

void main() {
    texcoord = gl_FragCoord.xy;

    vec3 inColor = texelFetch(colorBuffer, ivec2(texcoord), 0).xyz;

    // reinhard tone mapping
    vec3 mapped = inColor / (inColor + vec3(1));

    outColor = vec4(mapped, 1);
}
