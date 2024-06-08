# Vulkan Diagrams
Helpful links: [David-DiGioia](https://github.com/David-DiGioia/vulkan-diagrams?tab=readme-ov-file)

### Simplified flow
Source: [StackOverflow](https://stackoverflow.com/questions/39557141/what-is-the-difference-between-framebuffer-and-image-in-vulkan)

> `VkDeviceMemory` is just a sequence of N bytes in memory

> `VkImage` object adds to it e.g. information about the format (so you can address by texels, not bytes)

> `VkImageView` defines which part of `VkImage` to use

> `VkFramebuffer` binds a `VkImageView` with an attachment

> `VkRenderpass` defines which attachment will be drawn into

> `VkFramebuffer` + `VkRenderPass` defines the render target

![](../Utils/Images/SimplifiedFlow.png)

### Swap chain
![](../Utils/Images/VulkanDiagrams-SwapChain.jpg)

### Image view
![](../Utils/Images/VulkanDiagrams-ImageView.jpg)

### Frame buffer
![](../Utils/Images/VulkanDiagrams-FrameBuffer.jpg)

### Render pass 
![](../Utils/Images/VulkanDiagrams-RenderPass.jpg)

`Attachment`

![](../Utils/Images/VulkanDiagrams-RenderPassAttachment.jpg)

`Attachment reference`

![](../Utils/Images/VulkanDiagrams-RenderPassAttachmentReference.jpg)

`Subpass`

![](../Utils/Images/VulkanDiagrams-SubpassDescription.jpg)

`Subpass dependency`

![](../Utils/Images/VulkanDiagrams-SubpassDependency.jpg)

### Vertex data

`Pipeline input state`

![](../Utils/Images/VulkanDiagrams-VertexDataPipelineInput.jpg)

`Binding 0`

![](../Utils/Images/VulkanDiagrams-VertexDataBinding0.jpg)

`Binding 1`

![](../Utils/Images/VulkanDiagrams-VertexDataBinding1.jpg)

### Buffer

`Vertex buffer`

![](../Utils/Images/VulkanDiagrams-VertexBuffer.jpg)

`Index buffer`

![](../Utils/Images/VulkanDiagrams-IndexBuffer.jpg)

`Uniform buffer`

![](../Utils/Images/VulkanDiagrams-UniformBuffer.jpg)

### Descriptor 

`Descriptor set layout`

![](../Utils/Images/VulkanDiagrams-DescriptorSetLayout.jpg)

`Pipeline layout`

![](../Utils/Images/VulkanDiagrams-PipelineLayout.jpg)

`Descriptor pool`

![](../Utils/Images/VulkanDiagrams-DescriptorPool.jpg)

`Descriptor set`

![](../Utils/Images/VulkanDiagrams-DescriptorSet.jpg)

`Descriptor set update`

![](../Utils/Images/VulkanDiagrams-DescriptorSetUpdate.jpg)

### Pipeline
![](../Utils/Images/VulkanDiagrams-Pipeline.jpg)

### Cmd buffer

`To be submitted in graphics queue`

![](../Utils/Images/VulkanDiagrams-CmdBufferGraphics.jpg)

`To be submitted in transfer queue`

![](../Utils/Images/VulkanDiagrams-CmdBufferTransfer.jpg)

### Record buffer

`To be submitted in graphics queue`

![](../Utils/Images/VulkanDiagrams-GraphicsRecord.jpg)

`To be submitted in transfer queue`

![](../Utils/Images/VulkanDiagrams-TransferRecord.jpg)

### Graphics Ops
![](../Utils/Images/VulkanDiagrams-GraphicsOps.jpg)