#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

float getArea(vec2 a, vec2 b, vec2 c) {
    vec2 ab = b-a;
    vec2 ac = c-a;
    return abs(ab.x * ac.y - ab.y * ac.x) / 2;
}

layout (location = 0) in vec3 inPosWS[];
layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outPosWS;

void main() {
    vec4 inPos[3];
    for (int i = 0; i < 3; i++) {
        inPos[i] = gl_in[i].gl_Position.xyzw;
    }

    float area_xy = getArea(inPos[0].xy, inPos[1].xy, inPos[2].xy);
    float area_yz = getArea(inPos[0].yz, inPos[1].yz, inPos[2].yz);
    float area_xz = getArea(inPos[0].xz, inPos[1].xz, inPos[2].xz);

    int swizzelMode = 0;
    if (area_xy >= area_yz && area_xy >= area_xz)
        swizzelMode = 0;
    else if (area_yz >= area_xy && area_yz >= area_xz)
        swizzelMode = 1;
    else swizzelMode = 2;

    for (int i = 0; i < 3; i++) {
        switch (swizzelMode) {
            case 0: gl_Position = vec4(inPos[i].xy, 0, inPos[i].w); break;
            case 1: gl_Position = vec4(inPos[i].yz, 0, inPos[i].w); break;
            case 2: gl_Position = vec4(inPos[i].xz, 0, inPos[i].w); break;
        }
        outPos = inPos[i].xyz;
        outPosWS = inPosWS[i];
        EmitVertex();
    }

    EndPrimitive();
}

