# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

## Extension/
<pre>
    |<----------------------|{VKModelMgr}
    |
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
    |                       |ENEnum
    |                       :
    |                       :
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENUI


    |<----------------------|{VKModelMgr}
    |
    |<----------------------|{VKTextureImage}
    |
    |<----------------------|{VKVertexBuffer}
    |
    |<----------------------|{VKIndexBuffer}
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
    |<----------------------|ENUI
    |
    |<----------------------|ENSkyBox
    |
    |<----------------------|ENGrid
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