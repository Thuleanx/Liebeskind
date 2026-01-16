#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 posWS;

layout(binding = 1, rgba32f, set = 0) uniform image3D voxelTexture;

void main() {
    vec3 address = (pos * 0.5) + 0.5;

    vec4 outColor = vec4(address, 0);

    ivec3 physicalAddress = ivec3(imageSize(voxelTexture) * address);

    imageStore(voxelTexture, physicalAddress, outColor);
}
