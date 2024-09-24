# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    |<----------------------|{VKAttachment}
    |
    |<----------------------|{VKSubPass}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|{UIImpl}
    |
    |                       |ENEnum
    |                       :
    |                       :
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENUI


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


    |<----------------------|{VKDeviceMgr}
    |
    |<----------------------|{UIImpl}
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
    |<----------------------|{UIImpl}
    |
    |<......................|ENConfig
    |
    |
    |(protected)
    |ENCamera


    |<----------------------|VKInitSequence
    |
    |<----------------------|VKDrawSequence
    |
    |<----------------------|VKDeleteSequence
    |
    |<----------------------|ENUI
    |
    |<----------------------|ENGrid
    |
    |<----------------------|ENGeneric
    |
    |<----------------------|ENCamera
    |
    |
    |(protected)
    |ENApplication
    :
    :
    |main
</pre>