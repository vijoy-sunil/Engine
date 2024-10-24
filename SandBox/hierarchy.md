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