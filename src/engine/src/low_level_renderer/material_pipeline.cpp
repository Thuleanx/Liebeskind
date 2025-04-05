#include "low_level_renderer/material_pipeline.h"

#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/config.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/shader_data.h"
#include "low_level_renderer/vertex_buffer.h"

namespace graphics {
MaterialPipeline MaterialPipeline::create(
    vk::Device device,
    vk::ShaderModule vertexShader,
    vk::ShaderModule instanceRenderingVertexShader,
    vk::ShaderModule fragmentShader,
    vk::RenderPass renderPass,
    vk::SampleCountFlagBits msaaSampleCount
) {
    PipelineDescriptorData globalDescriptorData;
    {  // create global descriptor information
        const vk::DescriptorSetLayoutBinding globalSceneDataBinding(
            0,  // binding
            vk::DescriptorType::eUniformBuffer,
            1,  // descriptor count
            vk::ShaderStageFlagBits::eVertex |
                vk::ShaderStageFlagBits::eFragment
        );
        const std::array<vk::DescriptorSetLayoutBinding, 1> globalBindings = {
            globalSceneDataBinding
        };
        const vk::ResultValue<vk::DescriptorSetLayout>
            globalDescriptorSetLayoutCreation =
                device.createDescriptorSetLayout(
                    {{},
                     static_cast<uint32_t>(globalBindings.size()),
                     globalBindings.data()}
                );
        VULKAN_ENSURE_SUCCESS(
            globalDescriptorSetLayoutCreation.result,
            "Can't create global descriptor set layout"
        );
        std::vector<vk::DescriptorPoolSize> poolSizes = {vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1)};
        globalDescriptorData = {
            .setLayout = globalDescriptorSetLayoutCreation.value,
            .allocator = DescriptorAllocator::create(
                device,
                poolSizes,
                MAX_FRAMES_IN_FLIGHT
            )
        };
    }

    PipelineDescriptorData materialDescriptorData;
    {  // create material descriptor information
        const vk::ResultValue<vk::DescriptorSetLayout>
            materialDescriptorSetLayoutCreation =
                device.createDescriptorSetLayout(
                    {{},
                     static_cast<uint32_t>(MATERIAL_BINDINGS.size()),
                     MATERIAL_BINDINGS.data()}
                );
        VULKAN_ENSURE_SUCCESS(
            materialDescriptorSetLayoutCreation.result,
            "Can't create material descriptor set layout"
        );
        materialDescriptorData = {
            .setLayout = materialDescriptorSetLayoutCreation.value,
            .allocator = DescriptorAllocator::create(
                device,
                MATERIAL_DESCRIPTOR_POOL_SIZES,
                MAX_FRAMES_IN_FLIGHT
            )
        };
    }

    PipelineDescriptorData instanceRenderingDescriptorData;
    {
        const vk::DescriptorSetLayoutBinding instancedRenderingBinding(
            0,  // binding
            vk::DescriptorType::eStorageBuffer,
            1,  // descriptor count
            vk::ShaderStageFlagBits::eVertex
        );
        const std::array<vk::DescriptorSetLayoutBinding, 1>
            instancedRenderingBindings = {instancedRenderingBinding};
        const vk::ResultValue<vk::DescriptorSetLayout>
            instancedRenderingDescriptorSetLayoutCreation =
                device.createDescriptorSetLayout(
                    {{},
                     static_cast<uint32_t>(instancedRenderingBindings.size()),
                     instancedRenderingBindings.data()}
                );
        VULKAN_ENSURE_SUCCESS(
            instancedRenderingDescriptorSetLayoutCreation.result,
            "Can't create instancedRendering descriptor set layout"
        );

        std::vector<vk::DescriptorPoolSize> poolSizes = {vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)};
        instanceRenderingDescriptorData = {
            .setLayout = instancedRenderingDescriptorSetLayoutCreation.value,
            .allocator = DescriptorAllocator::create(
                device,
                poolSizes,
                MAX_FRAMES_IN_FLIGHT
            )
        };
    }

    const std::vector<vk::DescriptorSetLayout> instanceRenderingSetLayouts = {
        globalDescriptorData.setLayout,
        materialDescriptorData.setLayout,
        instanceRenderingDescriptorData.setLayout,
    };

    const std::vector<vk::DescriptorSetLayout> regularSetLayouts = {
        globalDescriptorData.setLayout,
        materialDescriptorData.setLayout,
    };

    const vk::PushConstantRange pushConstantRange(
        vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUPushConstants)
    );

    const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertexShader, "main"
    );
    const vk::PipelineShaderStageCreateInfo
        instanceRenderingVertexShaderStageInfo(
            {},
            vk::ShaderStageFlagBits::eVertex,
            instanceRenderingVertexShader,
            "main"
        );
    const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, fragmentShader, "main"
    );

    const std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    const vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
        {}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()
    );

    const auto bindingDescription = Vertex::getBindingDescription();
    const auto attributeDescription = Vertex::getAttributeDescriptions();
    const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo(
        {},
        1,
        &bindingDescription,
        static_cast<uint32_t>(attributeDescription.size()),
        attributeDescription.data()
    );
    const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo(
        {},
        vk::PrimitiveTopology::eTriangleList,
        vk::False  // primitive restart
    );
    const vk::PipelineViewportStateCreateInfo viewportStateInfo(
        {}, 1, nullptr, 1, nullptr
    );
    const vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo(
        {},
        vk::False,  // depth clamp enable. only useful for shadow mapping
        vk::False,  // rasterizerDiscardEnable
        vk::PolygonMode::eFill,  // fill polygon with fragments
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        vk::False,  // depth bias, probably useful for shadow mapping
        0.0f,
        0.0f,
        0.0f,
        1.0f  // line width
    );
    const vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
        {},
        msaaSampleCount,
        vk::False,
        1.0f,
        nullptr,
        vk::False,
        vk::False
    );
    const vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        vk::True,  // enable blend
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    const std::array<float, 4> colorBlendingConstants = {
        0.0f, 0.0f, 0.0f, 0.0f
    };
    const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
        {},
        vk::False,
        vk::LogicOp::eCopy,
        1,
        &colorBlendAttachment,
        colorBlendingConstants
    );
    const vk::PipelineDepthStencilStateCreateInfo depthStencilState(
        {},
        vk::True,  // enable depth test
        vk::True,  // enable depth write
        vk::CompareOp::eLess,
        vk::False,  // disable depth bounds test
        vk::False,  // disable stencil test
        {},         // stencil front op state
        {},         // stencil back op state
        {},         // min depth bound
        {}          // max depth bound
    );

    PipelineData regularPipeline;
    {
        const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
            {},
            regularSetLayouts.size(),
            regularSetLayouts.data(),
            1,
            &pushConstantRange
        );
        const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
            device.createPipelineLayout(pipelineLayoutInfo);
        VULKAN_ENSURE_SUCCESS(
            pipelineLayoutCreation.result, "Can't create pipeline layout:"
        );
        const vk::PipelineShaderStageCreateInfo shaderStages[] = {
            vertexShaderStageInfo, fragmentShaderStageInfo
        };
        vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
            {},
            2,
            shaderStages,
            &vertexInputStateInfo,
            &inputAssemblyStateInfo,
            nullptr,  // no tesselation viewport
            &viewportStateInfo,
            &rasterizerCreateInfo,
            &multisamplingInfo,
            &depthStencilState,
            &colorBlendingInfo,
            &dynamicStateInfo,
            pipelineLayoutCreation.value,
            renderPass,
            0
        );
        const vk::ResultValue<vk::Pipeline> pipelineCreation =
            device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
        VULKAN_ENSURE_SUCCESS(
            pipelineCreation.result, "Can't create graphics pipeline:"
        );

        regularPipeline = {
            .pipeline = pipelineCreation.value,
            .layout = pipelineLayoutCreation.value,
        };
    }

    PipelineData instanceRenderingPipeline;
    {
        const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
            {},
            instanceRenderingSetLayouts.size(),
            instanceRenderingSetLayouts.data(),
            0,
            nullptr
        );
        const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
            device.createPipelineLayout(pipelineLayoutInfo);
        VULKAN_ENSURE_SUCCESS(
            pipelineLayoutCreation.result, "Can't create pipeline layout:"
        );
        const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
            instanceRenderingVertexShaderStageInfo, fragmentShaderStageInfo
        };
        vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
            {},
            shaderStages.size(),
            shaderStages.data(),
            &vertexInputStateInfo,
            &inputAssemblyStateInfo,
            nullptr,  // no tesselation viewport
            &viewportStateInfo,
            &rasterizerCreateInfo,
            &multisamplingInfo,
            &depthStencilState,
            &colorBlendingInfo,
            &dynamicStateInfo,
            pipelineLayoutCreation.value,
            renderPass,
            0
        );
        const vk::ResultValue<vk::Pipeline> pipelineCreation =
            device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
        VULKAN_ENSURE_SUCCESS(
            pipelineCreation.result, "Can't create graphics pipeline:"
        );

        instanceRenderingPipeline = {
            .pipeline = pipelineCreation.value,
            .layout = pipelineLayoutCreation.value,
        };
    }

    return {
        .regularPipeline = regularPipeline,
        .instanceRenderingPipeline = instanceRenderingPipeline,
        .globalDescriptor = globalDescriptorData,
        .instanceRenderingDescriptor = instanceRenderingDescriptorData,
        .materialDescriptor = materialDescriptorData
    };
}

void MaterialPipeline::destroyBy(vk::Device device) const {
    destroy(globalDescriptor, device);
    destroy(instanceRenderingDescriptor, device);
    destroy(materialDescriptor, device);

    destroy(regularPipeline, device);
    destroy(instanceRenderingPipeline, device);
}

void destroy(const PipelineData& pipelineData, vk::Device device) {
    device.destroy(pipelineData.pipeline);
    device.destroy(pipelineData.layout);
}

void destroy(
    const PipelineDescriptorData& pipelineDescriptor, vk::Device device
) {
    pipelineDescriptor.allocator.destroyBy(device);
    device.destroy(pipelineDescriptor.setLayout);
}
}  // namespace Graphics
