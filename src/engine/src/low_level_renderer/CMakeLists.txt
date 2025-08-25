set(SRC 
    graphics_module.cpp
    render_submission.cpp
    material_pipeline.cpp
    pipeline_template.cpp
    materials.cpp
    meshes.cpp
    shaders.cpp
    graphics_device_interface.cpp 
    graphics_user_interface.cpp 
    descriptor_write_buffer.cpp 
    descriptor_allocator.cpp 
    renderpass_data.cpp
    swapchain_data.cpp
    vertex_buffer.cpp
    data_buffer.cpp
    instance_rendering.cpp
    queue_family.cpp
    texture.cpp
    private/shader_helper.cpp
    private/sampler.cpp
    private/command.cpp
    private/command.h
    private/image.cpp
    private/image.h
    private/graphics_device_helper.h
    private/graphics_device_helper.cpp
    private/validation.h
    private/validation.cpp
    private/swapchain.h
    private/swapchain.cpp
    private/buffer.h
    private/buffer.cpp
    private/bloom.h
    private/bloom.cpp
    private/descriptor.h
    private/descriptor.cpp
    private/pipeline.h
    private/pipeline.cpp
)

add_library(low_level_renderer ${SRC})

target_link_libraries(low_level_renderer PUBLIC third_party)
target_link_libraries(low_level_renderer PRIVATE resource_management)
target_link_libraries(low_level_renderer PRIVATE core)

