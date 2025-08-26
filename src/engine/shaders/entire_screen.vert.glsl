#version 450

layout(location = 0) out vec2 uv;

void main() {
    // this is so index will be [0,1,2,3,2,1] which correspond to 
    // two triangles covering the entire screen
    int index = min(gl_VertexIndex, 6 - gl_VertexIndex);
    vec2 texcoord = vec2(index & 1, index & 2);
    gl_Position = vec4(texcoord * 2 - 1, 0, 1);
}
