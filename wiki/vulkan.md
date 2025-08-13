# Vulkan is a hard mess

Here's notes about what I know about Vulkan as I work on it.
There will be occasional gaps on this project where I don't make progress for
multiple months, either due to working on actual game projects or feeling burnt
out. This document will serve as a refresher in case I forget everything about 
Vulkan, which is likely.

## Execution Model
Vulkan exposes one or more devices. For Liebeskind's purpose we only really use
one. 
Each device exposes one or more queues for asynchronous work. 
Each set of queues is partitioned into multiple families. 
Each family supports a type of functionality (graphics, compute, video decode,
video encode, memory management, transfer) and work for a specific family can be 
executed on any queue within that family.

Device memory is explicitly managed by the application.
Each device (think graphics card) can have one or more heaps of memory.
They are either device-local (physically connected to the device) or host-local
(physically connected to the computer, but not the device), however they are all
visible to the device.

Vulkan queues allow you to submit commands for execution. Specifically, they have 
a command buffer that can be recorded ahead of execution time, and submitted
to the queue for execution. Once submitted, it is fire and forget, although we  
can set synchronization constraints (semaphores and fences) to fix certain 
orderings of operations across queues.

## Object Model
When interfacing with Vulkan, every object is instead refered to by handles.
These are 8 bytes, and objects of different types might share the same handle value.
In Liebeskind, we try to use the C++ version of these handles, prefixed by `vk::`,
for type safety.

These objects are allocated from a `vk::Device`.
Once an object is created or allocated, its structure is immutable but content
can be changed.
They must be manually freed by `vk::Destroy` or `vk::Free`.

While object creation and destruction are expected to be low-frequency during runtime,
allocation and freeing can be done at high frequency.
Objects can be freed / destroyed in any order so long as the parent is 
freed/destroyed after its children and are not accessed after.

## Common Objects

Here is an outline of common objects as they appear in setting up the rendering 
pipeline.

### 1 - Instance and physical device selection
`vk::Instance` allows us to query Vulkan API for hardware support, and query /
select a `vk::PhysicalDevice` to perform work. 
`vk::PhysicalDevice`, for most purposes, is a graphics card.

### 2 - Logical device and queue families

After selecting the hardware device, we create a `vk::Device` (logical device).
This describes the device features we're using, which queue families we use, and 
will be our interface for creating Vulkan objects.

### 3 - Window surface and swapchain
To render to a window, we need a window surface (`vk::SurfaceKHR`) and a swapchain
`vk::SwapchainKHR`. A surface is a bridge to the window manager, a cross-platform 
abstraction over windows to render. 
Luckily, SDL handles this for us with SDL_Vulkan_CreateSurface.

A swapchain is a collection of render targets.
Its purpose is holding the image on screen, as well as the ones we're rendering to.
Every time we have to draw a frame, we must query the swap chain for an image to render to.
When we are finished, the image is returned to the chain for presentation.

In Liebeskind, we use triple buffering (for fun) but many applications use double 
buffering (a.k.a VSync).

### 4 - Image views and framebuffers
`vk::ImageView` references a specific part of the image to be used.
`vk::Framebuffer` is a collection of color, depth, and stencil targets to be used 
for rendering.
Typically, we create an image view and framebuffer for each image in the swapchain 
and use those at runtime, to prevent creating them at runtime.

### 5 - Render passes
`vk::RenderPass` describes the types of images used during the rendering operation,
how they should be used, and how their layouts should be transitioned between 
multiple phases of the rendering pipeline.
These describe the images we need, and the `vk::Framebuffer`s bind the actual images
to the correct slots.

### 6 - Graphics Pipeline
`vk::Pipeline` describes the configurable state of the graphics card, 
things like viewport size, depth buffer op, and programmable state using `vk::ShaderModule`.

All of this needs to be set in advance (at creation time), so switching to 
a different shader / slightly changing your vertex layout = creating a 
new graphics pipeline.
One can, however, create many `vk::Pipeline` objects in advance for the different
combinations you need.

### 7 - Command Pool and Command Buffers
`vk::CommandBuffer` describes a buffer of commands we can submit to a queue.
`vk::CommandPool` holds a collection of them for reuse.

### 8 - Vertex data and buffers
`vk::VertexInputAttributeDescription` describes your vertex input struct.
The vertex data itself lives on a `vk::Buffer`. When created, this doesn't have
any memory attached so we have to allocate some `vk::DeviceMemory` associated
with said buffer. 
This reserves memory on the graphics card associated with this buffer.

For Liebeskind, all meshes contains both a vertex buffer and index buffer.
Since the graphics card might have different memory with different functions, 
we actually don't push data directly to the vertex buffer, chosing instead to push
to a staging buffer with a host visibile + coherent memory, then transfer the data
into the vertex buffer that's device local.

### 9 - Descriptors
A descriptor allows shaders to access resources such as buffers and images. 
It consists of:
- `vk::DescriptorSetLayout` specifies the layout of the buffer / image resources 
that will be bound.
- `vk::DescriptorSet` actually stores the mapping to bound objects. This is so 
that we can have multiple descriptor sets for the same shared layout (think one
shader, different material parameters).
- `vk::DescriptorPool` is a pool of descriptor sets. 

There are many types of descriptors, but we primarily use `UniformBuffer` or 
`StorageBuffer`.
In Liebeskind, we have some objects that interacts with descriptors:
- `graphics::DescriptorAllocator` holds `vk::DescriptorPool` of different sizes.
It handles pool creation and allocating descriptor sets from pools
- `graphics::DescriptorWriteBuffer` is a buffer of binding commands that bind
buffers / images to different descriptor sets. This is so that they can be 
submitted as a batch.


