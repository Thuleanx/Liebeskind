#include "low_level_renderer/instance_rendering.h"

#include "core/logger/assert.h"

RenderInstanceID RenderInstanceManager::create(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DescriptorSetLayout setLayout,
    DescriptorAllocator& descriptorAllocator,
    DescriptorWriteBuffer& writeBuffer,
    uint16_t numberOfEntries
) {
    uint16_t index = renderInstances.size();
    renderInstances.emplace_back();
    auto descriptorSets =
        descriptorAllocator.allocate(device, setLayout, MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        StorageBuffer<InstanceData> storageBuffer =
            StorageBuffer<InstanceData>::create(
                device, physicalDevice, numberOfEntries
            );

        storageBuffer.bind(writeBuffer, descriptorSets[i], 0);
        renderInstances[index][i] = {
            .storageBuffer = storageBuffer,
            .descriptorSet = descriptorSets[i],
        };
    }
    return {index};
}

void RenderInstanceManager::bind(
    vk::CommandBuffer commandBuffer,
    vk::PipelineLayout layout,
    RenderInstanceID id,
    uint32_t currentFrame
) const {
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        layout,
        2,  // set
        1,
        &renderInstances.at(id.index).at(currentFrame).descriptorSet,
        0,
        nullptr
    );
}

void RenderInstanceManager::update(
    RenderInstanceID id,
    uint32_t currentFrame,
    std::span<const InstanceData> instanceData
) const {
    ASSERT(
        currentFrame < MAX_FRAMES_IN_FLIGHT,
        "Updating instance with frame "
            << currentFrame << " which is >= " << MAX_FRAMES_IN_FLIGHT
    );
    ASSERT(
        getCapacity(id) >= instanceData.size(),
        "Provided " << instanceData.size()
                    << " instances which exceeds the capacity of "
                    << getCapacity(id) << " for instance " << id.index
    );
    renderInstances.at(id.index)
        .at(currentFrame)
        .storageBuffer.update(instanceData);
}

uint16_t RenderInstanceManager::getCapacity(RenderInstanceID id) const {
    ASSERT(
        renderInstances.size() > id.index,
        "Expect a valid render instance containing a valid index that is < "
            << renderInstances.size() << " but got " << id.index
    );
    ASSERT(
        renderInstances[id.index].size() > 0,
        "Expects render instance to own at least one storage buffer"
    );
    return renderInstances[id.index][0].storageBuffer.dataCount;
}

void RenderInstanceManager::destroyBy(const vk::Device& device) const {
    for (const auto& instance : renderInstances)
        for (const RenderInstance& frameInstance : instance)
            frameInstance.storageBuffer.destroyBy(device);
}

bool RenderInstanceID::operator==(const RenderInstanceID& other) const {
    return index == other.index;
}

size_t RenderInstanceIDHashFunction::operator()(const RenderInstanceID& id
) const {
    return id.index;
}
