#include "radiance_cascade.h"

#include "core/logger/vulkan_ensures.h"
#include "descriptor.h"
#include "low_level_renderer/shader_data.h"
#include "low_level_renderer/shaders.h"
#include "low_level_renderer/vertex_buffer.h"
#include "low_level_renderer/texture.h"
#include "low_level_renderer/descriptor_write_buffer.h"

#include "image.h"

graphics::RadianceCascadeData graphics::create(const RadianceCascadeCreateInfo& info) {
    constexpr vk::Format imageFormat = vk::Format::eR32G32B32A32Sfloat;

    const UniformBuffer<glm::mat4> sceneDataBuffer = UniformBuffer<glm::mat4>::create(
        info.device, info.physicalDevice);
    sceneDataBuffer.update(glm::ortho(
        info.center.x - info.size.x,
        info.center.x + info.size.x,
        info.center.y - info.size.y,
        info.center.y + info.size.y,
        info.center.z - info.size.z,
        info.center.z + info.size.z
    ));

    auto createSDFTexture = [&info]() {
        const auto [image, memory] = Image::create(Image::CreateInfo {
            device: info.device,
            physicalDevice: info.physicalDevice,
            size: vk::Extent3D(info.resolution, info.resolution, info.resolution),
            format: imageFormat,
            usage: vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc
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
    };

    const std::array<Texture, 2> sdfTextures = {
        createSDFTexture(),
        createSDFTexture(),
    };

    const RadianceCascadeData::Rasterization rasterization = [&info, &sceneDataBuffer, &sdfTextures]() {
        const ShaderID vertex = loadShaderFromFile(
            info.shaders, info.device, "shaders/scene_voxelizer.vert.glsl.spv"
        );
        const ShaderID geometry = loadShaderFromFile(
            info.shaders, info.device, "shaders/scene_voxelizer.geom.glsl.spv"
        );
        const ShaderID fragment = loadShaderFromFile(
            info.shaders, info.device, "shaders/scene_voxelizer.frag.glsl.spv"
        );

        auto [renderPass, framebuffer] = [&info]() {
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

            const vk::Framebuffer emptyFramebuffer = [&info, &renderPass]() {
                const vk::FramebufferCreateInfo framebufferCreateInfo(
                    {},
                    renderPass,
                    0,
                    nullptr,
                    info.resolution,
                    info.resolution,
                    1
                );
                const vk::ResultValue<vk::Framebuffer> framebufferCreation =
                    info.device.createFramebuffer(framebufferCreateInfo);
                VULKAN_ENSURE_SUCCESS(
                    framebufferCreation.result, "Can't create main framebuffer:"
                );
                return framebufferCreation.value;
            }();

            return std::make_tuple(renderPass, emptyFramebuffer);
        }();

        const vk::DescriptorSetLayout setLayout = [&info, &sceneDataBuffer]() {
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
                vk::ShaderStageFlagBits::eFragment
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
        const vk::DescriptorPool pool = 
            createDescriptorPool(info.device, 1, poolSizes);
        const vk::DescriptorSet descriptorSet = [&info, &pool, &setLayout]() {
            const vk::ResultValue<std::vector<vk::DescriptorSet>>
                descriptorSetCreation = info.device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                    pool,
                    1,
                    &setLayout
                });
            VULKAN_ENSURE_SUCCESS(descriptorSetCreation.result, "Can't allocate descriptor set for radiance cascade rasterization step");
            ASSERT(descriptorSetCreation.value.size() == 1, "Can't allocate descriptor set for radiance cascade rasterization step");
            return descriptorSetCreation.value[0];
        }();

        sceneDataBuffer.bind(info.writeBuffer, descriptorSet, 0);
        info.writeBuffer.writeImage(
            descriptorSet,
            1,
            sdfTextures[0].imageView,
            vk::DescriptorType::eStorageImage,
            info.sampler,
            vk::ImageLayout::eGeneral
        );

        const vk::PipelineLayout pipelineLayout = [&info, &setLayout]() {
            const std::array<vk::DescriptorSetLayout, 1> setLayouts = {setLayout};
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

        const vk::Pipeline pipeline = [&info, &vertex, &fragment, &geometry, &pipelineLayout, &renderPass]() {
            const std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStages = {
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eVertex,
                    getModule(info.shaders, vertex),
                    "main"
                ),
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eGeometry,
                    getModule(info.shaders, geometry),
                    "main"
                ),
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eFragment,
                    getModule(info.shaders, fragment),
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

        return RadianceCascadeData::Rasterization {
            vertex,
            geometry,
            fragment,
            setLayout,
            descriptorSet,
            pool,
            pipelineLayout,
            pipeline,
            framebuffer,
            renderPass
        };
    }();

    const RadianceCascadeData::JumpFlood jumpFlood = [&info, &sdfTextures]() {
        const vk::DescriptorSetLayout setLayout = [&info]() {
            const std::array<vk::DescriptorSetLayoutBinding, 1> dataBindings = {
                vk::DescriptorSetLayoutBinding(
                    0,
                    vk::DescriptorType::eStorageImage,
                    1,
                    vk::ShaderStageFlagBits::eCompute
                ),
            };
            const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo(
                {},
                dataBindings.size(),
                dataBindings.data()
            );
            const vk::ResultValue<vk::DescriptorSetLayout> setLayoutCreation
                = info.device.createDescriptorSetLayout(layoutCreateInfo);
            VULKAN_ENSURE_SUCCESS(
                setLayoutCreation.result,
                "Can't create global descriptor set layout"
            );
            return setLayoutCreation.value;
        }();

        const std::array<vk::DescriptorPoolSize, 1> poolSizes = {
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 2),
        };
        const vk::DescriptorPool pool = 
            createDescriptorPool(info.device, 2, poolSizes);

        const std::array<vk::DescriptorSet, 2> textureSets = [&info, &pool, &setLayout]() {
            const std::array setLayouts = {setLayout, setLayout};
            const vk::ResultValue<std::vector<vk::DescriptorSet>>
                descriptorSetCreation = info.device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                    pool,
                    setLayouts.size(),
                    setLayouts.data()
                });
            ASSERT(descriptorSetCreation.value.size() == 2, "Can't allocate descriptor set for radiance cascade jump flood step");
            return std::array {descriptorSetCreation.value[0], descriptorSetCreation.value[1]};
        }();

        info.writeBuffer.writeImage(
            textureSets[0],
            0,
            sdfTextures[0].imageView,
            vk::DescriptorType::eStorageImage,
            info.sampler,
            vk::ImageLayout::eGeneral
        );
        info.writeBuffer.writeImage(
            textureSets[1],
            0,
            sdfTextures[1].imageView,
            vk::DescriptorType::eStorageImage,
            info.sampler,
            vk::ImageLayout::eGeneral
        );


        const ShaderID compute = loadShaderFromFile(
            info.shaders, info.device, "shaders/jump_flood.comp.glsl.spv"
        );

        const vk::PipelineLayout pipelineLayout = [&info, &setLayout, &compute]() {
            const std::array<vk::DescriptorSetLayout, 2> setLayouts = {setLayout, setLayout};

            const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
                {},
                setLayouts.size(),
                setLayouts.data(),
                0,
                nullptr
            );

            const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
                info.device.createPipelineLayout(pipelineLayoutInfo);
            VULKAN_ENSURE_SUCCESS(
                pipelineLayoutCreation.result, "Can't create pipeline layout:"
            );

            return pipelineLayoutCreation.value;
        }();

        const std::vector<vk::Pipeline> pipelines = [&info, &pipelineLayout, &compute]() {
            uint k = info.resolution == 1u ? 1u : 1u << (32u - __builtin_clzl(info.resolution-1));

            std::vector<vk::Pipeline> pipelines;
            pipelines.reserve(32);

            while (k > 1) {
                k /= 2;

                constexpr std::array<vk::SpecializationMapEntry, 1> SPECIALIZATION_INFO = {
                    vk::SpecializationMapEntry{0, 
                        0,
                        sizeof(uint)},
                };
                const vk::SpecializationInfo specializationInfo(
                    SPECIALIZATION_INFO.size(),
                    SPECIALIZATION_INFO.data(),
                    sizeof(uint),
                    &k
                );

                const vk::ResultValue<vk::Pipeline> pipelineCreation = info.device.createComputePipeline(
                    nullptr,
                    vk::ComputePipelineCreateInfo(
                        {},
                        vk::PipelineShaderStageCreateInfo(
                            {},
                            vk::ShaderStageFlagBits::eCompute,
                            getModule(info.shaders, compute),
                            "main",
                            &specializationInfo
                        ), pipelineLayout)
                );

                VULKAN_ENSURE_SUCCESS(
                    pipelineCreation.result, "Can't create graphics pipeline:"
                );
                pipelines.push_back(pipelineCreation.value);
            }

            return pipelines;
        }();

        return RadianceCascadeData::JumpFlood {
            compute,
            setLayout,
            pool,
            textureSets,
            pipelineLayout,
            pipelines
        };
    }();

    return RadianceCascadeData {
        .sceneDataBuffer = sceneDataBuffer,
        .center = info.center,
        .size = info.size,
        .resolution = info.resolution,
        .sdfTextures = sdfTextures,
        .rasterization = rasterization,
        .jumpFlood = jumpFlood
    };
}

void graphics::recordDraw(
    RadianceCascadeData& cascadeData,
    const RenderSubmission& renderSubmission,
    vk::CommandBuffer buffer,
	const RenderInstanceManager& instanceManager,
	const MaterialPipeline& pipelines,
	const MaterialStorage& materials,
	const MeshStorage& meshes,
	uint32_t currentFrame
) {
    std::array<vk::ImageMemoryBarrier, 1> barriers = {vk::ImageMemoryBarrier(
        {},
        vk::AccessFlagBits::eShaderWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        cascadeData.sdfTextures[0].image,
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, 
            0,
            1,
            0,
            1)
    )};
    buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, 
        vk::PipelineStageFlagBits::eFragmentShader, 
        {}, {}, {}, barriers);

    const vk::RenderPassBeginInfo sdfRenderpassInfo(
        cascadeData.rasterization.renderPass,
        cascadeData.rasterization.framebuffer,
        vk::Rect2D({}, vk::Extent2D(cascadeData.resolution, cascadeData.resolution)),
        0, nullptr
    );
    buffer.beginRenderPass(sdfRenderpassInfo, vk::SubpassContents::eInline);
    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        cascadeData.rasterization.pipelineLayout,
		static_cast<int>(MainPipelineDescriptorSetBindingPoint::eGlobal),
        1,
        &cascadeData.rasterization.set,
        0, nullptr
    );

    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, cascadeData.rasterization.pipeline);
    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        cascadeData.rasterization.pipelineLayout,
        0,
        1,
        &cascadeData.rasterization.set,
        0,
        nullptr
    );

    std::optional<MaterialInstanceID> boundMaterial = std::nullopt;
    for (const auto& [variant, transform, materialID, mesh] : renderSubmission.renderObjects) {
        const bool shouldBindMaterial =
            !boundMaterial.has_value() || boundMaterial.value() != materialID;
        if (shouldBindMaterial) {
            boundMaterial = materialID;
        }

        GPUPushConstants pushConstants = {.model = transform};
        buffer.pushConstants(
            cascadeData.rasterization.pipelineLayout,
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(GPUPushConstants),
            &pushConstants
        );
        bind(meshes, buffer, mesh);
        draw(meshes, buffer, mesh);
    }

	buffer.endRenderPass();
}

void graphics::destroy(const RadianceCascadeData& data, vk::Device device) {
    device.destroyFramebuffer(data.rasterization.framebuffer);
    device.destroyPipeline(data.rasterization.pipeline);
    device.destroyPipelineLayout(data.rasterization.pipelineLayout);
    device.destroyDescriptorPool(data.rasterization.pool);
    device.destroyDescriptorSetLayout(data.rasterization.setLayout);
    device.destroyRenderPass(data.rasterization.renderPass);

    for (const vk::Pipeline pipeline : data.jumpFlood.pipelines)
        device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(data.jumpFlood.pipelineLayout);
    device.destroyDescriptorPool(data.jumpFlood.pool);
    device.destroyDescriptorSetLayout(data.jumpFlood.setLayout);

    destroy(data.sdfTextures, device);
    data.sceneDataBuffer.destroyBy(device);
}
