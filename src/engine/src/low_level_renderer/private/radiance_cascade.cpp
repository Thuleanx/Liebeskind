#include "radiance_cascade.h"

#include "core/logger/vulkan_ensures.h"
#include "descriptor.h"
#include "low_level_renderer/shaders.h"
#include "low_level_renderer/vertex_buffer.h"
#include "low_level_renderer/texture.h"

#include "image.h"

graphics::RadianceCascadeData graphics::create(const RadianceCascadeCreateInfo& info) {
    constexpr vk::Format imageFormat = vk::Format::eR32G32B32A32Sfloat;

    const Texture sdfTexture = [&info]() {
        const auto [image, memory] = Image::create(Image::CreateInfo {
            device: info.device,
            physicalDevice: info.physicalDevice,
            size: vk::Extent3D(info.resolution, info.resolution, info.resolution),
            format: imageFormat,
            usage: vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
        });
        const vk::ImageView imageView =
            Image::createImageView(
                info.device,
                image,
                vk::ImageViewType::e3D,
                imageFormat,
                vk::ImageAspectFlagBits::eColor,
                0,
                1
            );
        return Texture {
            image: image,
            imageView: imageView,
            memory: memory,
            format: imageFormat,
        };
    }();


    const RadianceCascadeData::SDFData sdf = [&info, &sdfTexture]() {
        const vk::SubpassDescription subpass(
            {},
            vk::PipelineBindPoint::eGraphics
        );

        const vk::RenderPassCreateInfo renderPassInfo(
            {},
            0, nullptr,
            1, &subpass, 
            0, nullptr
        );

        const vk::ResultValue<vk::RenderPass> renderPassCreation = info.device.createRenderPass(renderPassInfo);
        VULKAN_ENSURE_SUCCESS(renderPassCreation.result, "Can't create renderpass for radiance cascade");
        const vk::RenderPass renderPass = renderPassCreation.value;

        const ShaderID vertexShaderID = loadShaderFromFile(
            info.shaders, info.device, "shaders/scene_voxelizer.vert.glsl.spv"
        );
        const ShaderID geometryShaderID = loadShaderFromFile(
            info.shaders, info.device, "shaders/scene_voxelizer.geom.glsl.spv"
        );
        const ShaderID fragmentShaderID = loadShaderFromFile(
            info.shaders, info.device, "shaders/scene_voxelizer.frag.glsl.spv"
        );

        const vk::DescriptorSetLayout globalSetLayout = [&info]() {
            const vk::DescriptorSetLayoutBinding sceneDataBinding(
                0,
                vk::DescriptorType::eUniformBuffer,
                1,
                vk::ShaderStageFlagBits::eVertex
            );

            const vk::DescriptorSetLayoutBinding renderResultBinding(
                1,
                vk::DescriptorType::eStorageImage,
                1,
                vk::ShaderStageFlagBits::eVertex
            );

            const std::array<vk::DescriptorSetLayoutBinding, 2> globalDataBindings = {
                sceneDataBinding,
                renderResultBinding,
            };
            const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo(
                {},
                globalDataBindings.size(),
                globalDataBindings.data()
            );
            const vk::ResultValue<vk::DescriptorSetLayout> globalSetLayoutCreation
                = info.device.createDescriptorSetLayout(layoutCreateInfo);
            VULKAN_ENSURE_SUCCESS(
                globalSetLayoutCreation.result,
                "Can't create global descriptor set layout"
            );
            return globalSetLayoutCreation.value;
        }();

        const std::array<vk::DescriptorPoolSize, 2> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
        };
        const vk::DescriptorPool globalPool = 
            createDescriptorPool(info.device, 1, poolSizes);
        const vk::DescriptorSet globalDescriptorSet = [&info, &globalPool, &globalSetLayout]() {
            const vk::ResultValue<std::vector<vk::DescriptorSet>>
                descriptorSetCreation = info.device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                    globalPool,
                    1,
                    &globalSetLayout
                });

            return descriptorSetCreation.value[0];
        }();

        const vk::PipelineLayout pipelineLayout = [&info, &globalSetLayout]() {
            const std::array<vk::DescriptorSetLayout, 1> setLayouts = {
                globalSetLayout
            };

            const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)
            );

            const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
                {},
                setLayouts.size(),
                setLayouts.data(),
                1,
                &pushConstantRange
            );

            const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
                info.device.createPipelineLayout(pipelineLayoutInfo);
            VULKAN_ENSURE_SUCCESS(
                pipelineLayoutCreation.result, "Can't create pipeline layout:"
            );

            return pipelineLayoutCreation.value;
        }();

        const vk::Pipeline pipeline = [&info, &vertexShaderID, &fragmentShaderID, &geometryShaderID, &pipelineLayout, &renderPass]() {
            const std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages = {
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eVertex,
                    getModule(info.shaders, vertexShaderID),
                    "main"
                ),
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eGeometry,
                    getModule(info.shaders, geometryShaderID),
                    "main"
                ),
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eFragment,
                    getModule(info.shaders, fragmentShaderID),
                    "main"
                ),
            };

            const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo(
                {},
                vk::PrimitiveTopology::eTriangleList,
                vk::False  // primitive restart
            );

            const auto vertexInputBinding = Vertex::getBindingDescription();
            const auto vertexInputAttributes = Vertex::getAttributeDescriptions();
            const std::vector<vk::VertexInputAttributeDescription>
                vertexInputAttributesVector = [&]() {
                    std::vector<vk::VertexInputAttributeDescription> result;
                    result.reserve(vertexInputAttributes.size());
                    for (const vk::VertexInputAttributeDescription& inputAttribute :
                        vertexInputAttributes)
                        result.push_back(inputAttribute);
                    return result;
                }();
            const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo(
                {},
                1,
                &vertexInputBinding,
                static_cast<uint32_t>(vertexInputAttributes.size()),
                vertexInputAttributes.data()
            );

            const vk::PipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterizationInfo(
                {},
                vk::ConservativeRasterizationModeEXT::eOverestimate,
                0
            );
            const vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo(
                {},
                vk::False,
                vk::False,
                vk::PolygonMode::eFill,
                vk::CullModeFlagBits::eBack,
                vk::FrontFace::eCounterClockwise,
                vk::False,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                &conservativeRasterizationInfo
            );

            const vk::Viewport viewport(
                0, 0, 
                info.resolution,
                info.resolution,
                0, 1);
            const vk::Rect2D scissor(
                vk::Offset2D(),
                vk::Extent2D(info.resolution, info.resolution)
            );
            const vk::PipelineViewportStateCreateInfo viewportStateInfo(
                {}, 1, &viewport, 1, &scissor
            );

            const vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
                {},
                vk::SampleCountFlagBits::e1,
                vk::False,
                0.0f,
                nullptr,
                vk::False,
                vk::False
            );

            const vk::PipelineDepthStencilStateCreateInfo depthStencilState(
                {},
                vk::False,
                vk::False,
                vk::CompareOp::eLess,
                vk::False,
                vk::False,
                {},
                {},
                {},
                {}
            );

            const std::array<float, 4> colorBlendingConstants = {
                0.0f, 0.0f, 0.0f, 0.0f
            };

            const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
                {},
                vk::False,
                vk::LogicOp::eCopy,
                0,
                nullptr,
                colorBlendingConstants
            );

            const vk::ResultValue<vk::Pipeline> pipelineCreation =
                info.device.createGraphicsPipeline(nullptr, 
                        vk::GraphicsPipelineCreateInfo(
                        {},
                        shaderStages.size(),
                        shaderStages.data(),
                        &vertexInputStateInfo,
                        &inputAssemblyStateInfo,
                        nullptr,
                        &viewportStateInfo,
                        &rasterizerCreateInfo,
                        &multisamplingInfo,
                        &depthStencilState,
                        &colorBlendingInfo,
                        nullptr,
                        pipelineLayout,
                        renderPass,
                        0
                    ));
            VULKAN_ENSURE_SUCCESS(
                pipelineCreation.result, "Can't create graphics pipeline:"
            );
            return pipelineCreation.value;
        }();

        return RadianceCascadeData::SDFData {
            .vertexShader = vertexShaderID,
            .geometryShader = geometryShaderID,
            .fragmentShader = fragmentShaderID,
            .globalSetLayout = globalSetLayout,
            .globalSetPool = globalPool,
            .globalSet = globalDescriptorSet,
            .pipelineLayout = pipelineLayout,
            .pipeline = pipeline,
            .renderPass = renderPassCreation.value,
            .texture = sdfTexture
        };
    }();

    return RadianceCascadeData {
        .center = info.center,
        .size = info.size,
        .sdf = sdf,
    };
}

void graphics::destroy(const RadianceCascadeData& data, vk::Device device) {
    device.destroyPipeline(data.sdf.pipeline);
    device.destroyPipelineLayout(data.sdf.pipelineLayout);
    device.destroyDescriptorPool(data.sdf.globalSetPool);
    device.destroyDescriptorSetLayout(data.sdf.globalSetLayout);
    device.destroyRenderPass(data.sdf.renderPass);
    destroy(std::array{data.sdf.texture}, device);

}
