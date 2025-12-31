# Star Judgement | Liebeskind

This is Thuleanx's game engine project

## Tasks
Here are the tasks I'm working on.
- [x] Bloom

## Render Pipeline
Currently, the flow of Liebeskind's render pipeline is as follows:

- Main Renderpass:
Renders to the <MultisampleColorBuffer> and <DepthBuffer>, then resolve into 
<BloomColorBuffer>::mip1
- Bloom Downsample Renderpass:
Each step is a separate renderpass.
Bloom downsample from mip to next mip. 
Bloom upsample from mip to previous mip, until the last step.
The last step upsample from <BloomColorBuffer>::mip1 and mixes it with <BloomColorBuffer>::mip0, 
then store into <IntermediateColorBuffer>
- Post Processing Renderpass:
From <IntermediateColorBuffer> to <SwapchainColorBuffer>

## Vulkan Overview
[vulkan](./vulkan.md)
