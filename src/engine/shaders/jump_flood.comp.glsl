#version 450

layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout (binding = 0, rgba32f, set = 0) uniform readonly image3D inputImage;
layout (binding = 0, rgba32f, set = 1) uniform writeonly image3D outputImage;

layout(constant_id = 0) const uint k = 1;

void main() {
    ivec3 dims = imageSize(inputImage);
    ivec3 texel = ivec3(gl_GlobalInvocationID.xyz);
    bool outOfBounds = 
        texel.x >= dims.x ||
        texel.y >= dims.y ||
        texel.z >= dims.z;
    if (outOfBounds) return;

    vec3 uv = (vec3(texel) + 0.5) / dims;

    // just any value very far away
    vec3 closest = vec3(-100, -100, -100);
    float closestSqDist = 1000000;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                ivec3 sampleUV = texel + ivec3(dx * k, dy * k, dz * k);

                bool isInImage = 
                    sampleUV.x >= 0 && sampleUV.x <= dims.x &&
                    sampleUV.y >= 0 && sampleUV.y <= dims.y &&
                    sampleUV.z >= 0 && sampleUV.z <= dims.z;

                if (isInImage) {
                    vec3 pos = imageLoad(inputImage, sampleUV).xyz;
                    float sqDist = dot(uv - pos, uv - pos);

                    if (sqDist < closestSqDist) {
                        closestSqDist = sqDist;
                        closest = pos;
                    }
                }
            }
        }
    }

    vec4 store = vec4(closest, closestSqDist);
    imageStore(outputImage, texel, store);
}
