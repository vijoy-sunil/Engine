# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

## Extension/
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
</pre>

## Controller/
<pre>
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
</pre>

## SandBox/
<pre>
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