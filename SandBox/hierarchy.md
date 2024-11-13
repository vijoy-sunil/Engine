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
    |<----------------------|{VKCmdBuffer}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|{VKSyncObject}
    |
    |                       |ENEnum
    |                       :
    |                       :
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENSkyBox


    |<----------------------|{VKVertexInput}
    |
    |<----------------------|{VKShaderStage}
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


    |<----------------------|{VKModelMgr}
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