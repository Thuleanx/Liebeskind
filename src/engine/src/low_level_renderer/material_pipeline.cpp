#include "low_level_renderer/material_pipeline.h"

#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/config.h"
#include "low_level_renderer/shader_data.h"
#include "low_level_renderer/vertex_buffer.h"

MaterialPipeline MaterialPipeline::create(
    PipelineType pipelineType,
    vk::Device device,
    vk::ShaderModule vertexShader,
    vk::ShaderModule fragmentShader,
    vk::RenderPass renderPass
) {
    std::array<
        vk::DescriptorSetLayout,
        static_cast<size_t>(PipelineSetType::MAX) + 1>
        descriptorSetLayouts;
    std::array<
        DescriptorAllocator,
        static_cast<size_t>(PipelineSetType::MAX) + 1>
        descriptorAllocators;

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
        descriptorSetLayouts[static_cast<size_t>(PipelineSetType::GLOBAL)] =
            globalDescriptorSetLayoutCreation.value;
        descriptorAllocators[static_cast<size_t>(PipelineSetType::GLOBAL)] =
            DescriptorAllocator::create(
                device,
                {vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1)},
                MAX_FRAMES_IN_FLIGHT
            );
    }

    {  // create material descriptor information
        const vk::DescriptorSetLayoutBinding materialPropertiesBinding(
            0,  // binding
            vk::DescriptorType::eUniformBuffer,
            1,  // descriptor count
            vk::ShaderStageFlagBits::eFragment
        );
        const vk::DescriptorSetLayoutBinding albedoBinding(
            1,  // binding
            vk::DescriptorType::eCombinedImageSampler,
            1,  // descriptor count
            vk::ShaderStageFlagBits::eFragment
        );
        const std::array<vk::DescriptorSetLayoutBinding, 2> materialBindings = {
            materialPropertiesBinding, albedoBinding
        };
        const vk::ResultValue<vk::DescriptorSetLayout>
            materialDescriptorSetLayoutCreation =
                device.createDescriptorSetLayout(
                    {{},
                     static_cast<uint32_t>(materialBindings.size()),
                     materialBindings.data()}
                );
        VULKAN_ENSURE_SUCCESS(
            materialDescriptorSetLayoutCreation.result,
            "Can't create material descriptor set layout"
        );
        descriptorSetLayouts[static_cast<size_t>(PipelineSetType::MATERIAL)] =
            materialDescriptorSetLayoutCreation.value;
        descriptorAllocators[static_cast<size_t>(PipelineSetType::MATERIAL)] =
            DescriptorAllocator::create(
                device,
                {
                    vk::DescriptorPoolSize(
                        vk::DescriptorType::eUniformBuffer, 1
                    ),
                    vk::DescriptorPoolSize(
                        vk::DescriptorType::eCombinedImageSampler, 1
                    ),
                },
                MAX_FRAMES_IN_FLIGHT
            );
    }

    if (pipelineType == PipelineType::INSTANCED) {
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

        descriptorSetLayouts[static_cast<size_t>(
            PipelineSetType::INSTANCE_RENDERING
        )] = instancedRenderingDescriptorSetLayoutCreation.value;
        descriptorAllocators[static_cast<size_t>(
            PipelineSetType::INSTANCE_RENDERING
        )] =
            DescriptorAllocator::create(
                device,
                {vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)},
                MAX_FRAMES_IN_FLIGHT
            );
    }

    std::vector<vk::DescriptorSetLayout> activeDescriptorSetLayouts;
    if (pipelineType == PipelineType::INSTANCED) {
        activeDescriptorSetLayouts = {
            descriptorSetLayouts[static_cast<size_t>(PipelineSetType::GLOBAL)],
            descriptorSetLayouts[static_cast<size_t>(PipelineSetType::MATERIAL
            )],
            descriptorSetLayouts
                [static_cast<size_t>(PipelineSetType::INSTANCE_RENDERING)],
        };
    } else {
        activeDescriptorSetLayouts = {
            descriptorSetLayouts[static_cast<size_t>(PipelineSetType::GLOBAL)],
            descriptorSetLayouts[static_cast<size_t>(PipelineSetType::MATERIAL
            )],
        };
    }
    vk::PushConstantRange pushConstantRange(
        vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUPushConstants)
    );
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},
        activeDescriptorSetLayouts.size(),
        activeDescriptorSetLayouts.data(),
        pipelineType != PipelineType::INSTANCED,
        pipelineType == PipelineType::INSTANCED ? nullptr : &pushConstantRange
    );
    const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
        device.createPipelineLayout(pipelineLayoutInfo);
    VULKAN_ENSURE_SUCCESS(
        pipelineLayoutCreation.result, "Can't create pipeline layout:"
    );
    const vk::PipelineLayout pipelineLayout = pipelineLayoutCreation.value;

    const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertexShader, "main"
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
    const vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertexShaderStageInfo, fragmentShaderStageInfo
    };
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
        vk::SampleCountFlagBits::e1,
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
        pipelineLayout,
        renderPass,
        0
    );
    const vk::ResultValue<vk::Pipeline> pipelineCreation =
        device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
    VULKAN_ENSURE_SUCCESS(
        pipelineCreation.result, "Can't create graphics pipeline:"
    );
    const vk::Pipeline pipeline = pipelineCreation.value;

    return {
        pipeline,
        pipelineLayout,
        descriptorSetLayouts,
        descriptorAllocators,
        pipelineType
    };
}

void MaterialPipeline::destroyBy(vk::Device device) const {
    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(layout);
    for (size_t setType = 0;
         setType <= static_cast<size_t>(PipelineSetType::MAX);
         setType++) {
        if (static_cast<PipelineSetType>(setType) ==
                PipelineSetType::INSTANCE_RENDERING &&
            pipelineType != PipelineType::INSTANCED)
            continue;
        descriptorAllocators[setType].destroyBy(device);
        device.destroyDescriptorSetLayout(descriptorSetLayouts[setType]);
    }
}
