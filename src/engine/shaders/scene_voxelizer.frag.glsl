#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 posWS;

layout(binding = 1, rgba32f, set = 0) uniform image3D voxelTexture;

void main() {
    vec4 outColor = vec4(posWS, 1);
    vec3 address = (pos * 0.5) + 0.5;
    ivec3 physicalAddress = ivec3(imageSize(voxelTexture) * address);

    imageStore(voxelTexture, physicalAddress, outColor);
}
