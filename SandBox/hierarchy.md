# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

## Extension/
<pre>
    |<----------------------|{VKInstanceData}
    |
    |<----------------------|{VKTextureImage}
    |
    |<----------------------|{VKVertexBuffer}
    |
    |<----------------------|{VKIndexBuffer}
    |
    |<----------------------|VKUniformBuffer
    |
    |<----------------------|{VKVertexInput}
    |
    |<----------------------|{VKShaderStage}
    |
    |<----------------------|{VKRasterization}
    |
    |<----------------------|{VKDepthStencil}
    |
    |<----------------------|{VKDescriptorSetLayout}
    |
    |<----------------------|{VKPushConstantRange}
    |
    |<----------------------|{VKPipelineLayout}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |                       |ENEnum
    |                       :
    |                       :
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENSkyBox


    |<----------------------|{VKInstanceData}
    |
    |<----------------------|{VKVertexBuffer}
    |
    |<----------------------|{VKIndexBuffer}
    |
    |<----------------------|{VKStorageBuffer}
    |
    |<----------------------|{VKVertexInput}
    |
    |<----------------------|{VKShaderStage}
    |
    |<----------------------|{VKRasterization}
    |
    |<----------------------|{VKDescriptorSetLayout}
    |
    |<----------------------|{VKPushConstantRange}
    |
    |<----------------------|{VKPipelineLayout}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKDescriptor}
    |
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENAnchor


    |<----------------------|{VKVertexInput}
    |
    |<----------------------|{VKShaderStage}
    |
    |<----------------------|{VKPushConstantRange}
    |
    |<----------------------|{VKPipelineLayout}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKSceneMgr}
    |
    |<......................|VKUniform
    |
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENGrid


    |<----------------------|{VKAttachment}
    |
    |<----------------------|{VKSubPass}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|UIImpl
    |
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENUI
</pre>

## Controller/
<pre>
    |<----------------------|{VKDeviceMgr}
    |
    |<----------------------|{UIInput}
    |
    |<----------------------|{UIUtil}
    |
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENGeneric


    |<----------------------|{VKModelMatrix}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{UIInput}
    |
    |<----------------------|{UIUtil}
    |
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENCamera
</pre>

## SandBox/
<pre>
    |<......................|ENEnum
    :
    :
    |ENLogHelper


    |<----------------------|VKInitSequence
    |
    |<----------------------|VKDrawSequence
    |
    |<----------------------|VKDeleteSequence
    |
    |<----------------------|ENSkyBox
    |
    |<----------------------|ENAnchor
    |
    |<----------------------|ENGrid
    |
    |<----------------------|ENUI
    |
    |<----------------------|ENGeneric
    |
    |
    |(protected)
    |ENApplication
    :
    :
    |main
</pre>