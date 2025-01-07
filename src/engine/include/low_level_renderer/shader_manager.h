#pragma once

#include <vulkan/vulkan.hpp>

struct ShaderID {
    // surely we won't need that many shaders
    uint8_t index;
};

class ShaderManager {
   public:
    [[nodiscard]]
    ShaderID load(vk::Device device, const char* filePath);
    vk::ShaderModule getModule(ShaderID id) const;
    void destroyBy(vk::Device device);

   private:
    uint8_t numberOfShaders;
    std::array<vk::ShaderModule, 1 << 8> shaders;
};
