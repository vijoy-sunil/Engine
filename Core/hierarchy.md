# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    |VKConfig, Log
    :
    :
    |{VKDeviceMgr}
    |(protected)
    |
    |---------------------->|VKWindow
    |
    |---------------------->|VKSurface
    |
    |                       |VKEnum
    |                       :
    |                       :
    |                       |VKLogHelper
    |                       :
    |                       :
    |---------------------->|VKQueue
    |                       |(protected)
    |                       |
    |                       |---------------------->|VKPhyDevice
    |
    |---------------------->|{VKValidation}
                            |(protected)
                            |
                            |---------------------->|VKInstance
                            |
                            |                       |{VKPhyDevice}
                            |                       |(protected)
                            |                       |
                            |                       |
                            |---------------------->|VKLogDevice


    |VKConfig, Log
    :
    :
    |VKVertexData
    |(protected)
    |
    |
    |{VKModelMgr}           |<......................|VKUniform
    |(protected)
    |
    |
    |VKModelMatrix
    |(protected)
    |
    |
    |VKInstanceData


    |{VKPhyDevice}
    |(protected)
    |
    |
    |{VKImageMgr}
    |(protected)
    |
    |---------------------->|VKSwapChainImage
    |
    |                       |{VKBufferMgr}
    |                       |(protected)
    |                       |
    |                       |
    |---------------------->|VKTextureImage
    |
    |---------------------->|VKDepthImage
    |
    |---------------------->|VKMultiSampleImage


    |{VKPhyDevice}
    |(protected)
    |
    |
    |{VKBufferMgr}
    |(protected)
    |
    |---------------------->|VKVertexBuffer
    |
    |---------------------->|VKIndexBuffer
    |
    |---------------------->|VKUniformBuffer
    |
    |---------------------->|VKStorageBuffer


    |{VKDeviceMgr}
    |(protected)
    |
    |
    |{VKRenderPassMgr}      |{VKImageMgr}
    |(protected)            |(protected)
    |                       |
    |                       |
    |---------------------->|VKAttachment
    |
    |---------------------->|VKSubPass
    |
    |---------------------->|VKFrameBuffer


    |{VKRenderPassMgr}
    |(protected)
    |
    |
    |{VKPipelineMgr}
    |(protected)
    |
    |---------------------->|VKVertexInput
    |
    |---------------------->|VKInputAssembly
    |
    |---------------------->|VKShaderStage
    |
    |---------------------->|VKViewPort
    |
    |---------------------->|VKRasterization
    |
    |                       |{VKImageMgr}
    |                       |(protected)
    |                       |
    |                       |
    |---------------------->|VKMultiSample
    |
    |---------------------->|VKDepthStencil
    |
    |---------------------->|VKColorBlend
    |
    |---------------------->|VKDynamicState
    |
    |---------------------->|VKDescriptorSetLayout
    |
    |---------------------->|VKPushConstantRange
    |
    |---------------------->|VKPipelineLayout


    |<----------------------|{VKDeviceMgr}
    |                       |(protected)
    |
    |VKCmdBuffer


    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKBufferMgr}
    |
    |<----------------------|{VKPipelineMgr}
    |
    |(protected)
    |VKCmd


    |<----------------------|{VKDeviceMgr}
    |                       |(protected)
    |
    |
    |VKCameraMgr


    |<----------------------|{VKDeviceMgr}
    |                       |(protected)
    |
    |<......................|VKLogHelper
    |
    |
    |VKSyncObject


    |<----------------------|{VKSwapChainImage}
    |
    |<----------------------|{VKDepthImage}
    |
    |<----------------------|{VKMultiSampleImage}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|{VKSceneMgr}
    |
    |(protected)
    |VKResizing


    |VKConfig, Log
    :
    :
    |{VKSceneMgr}           |{VKDeviceMgr}
    |(protected)            |(protected)
    |                       |
    |                       |
    |---------------------->|VKTextureSampler
    |
    |                       |{VKPipelineMgr}
    |                       |(protected)
    |                       |
    |                       |
    |---------------------->|VKDescriptor


    |<----------------------|{VKWindow}
    |
    |<----------------------|{VKInstance}
    |
    |<----------------------|{VKSurface}
    |
    |<----------------------|{VKLogDevice}
    |
    |<----------------------|VKInstanceData
    |
    |<----------------------|{VKSwapChainImage}
    |
    |<----------------------|VKTextureImage
    |
    |<----------------------|{VKDepthImage}
    |
    |<----------------------|{VKMultiSampleImage}
    |
    |<----------------------|VKVertexBuffer
    |
    |<----------------------|VKIndexBuffer
    |
    |<----------------------|{VKStorageBuffer}
    |
    |<----------------------|{VKAttachment}
    |
    |<----------------------|{VKSubPass}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|{VKVertexInput}
    |
    |<----------------------|VKInputAssembly
    |
    |<----------------------|{VKShaderStage}
    |
    |<----------------------|VKViewPort
    |
    |<----------------------|VKRasterization
    |
    |<----------------------|VKMultiSample
    |
    |<----------------------|VKDepthStencil
    |
    |<----------------------|VKColorBlend
    |
    |<----------------------|VKDynamicState
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
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|{VKSyncObject}
    |
    |
    |(protected)
    |VKInitSequence


    |<----------------------|{VKWindow}
    |
    |<----------------------|{VKModelMgr}
    |
    |<----------------------|{VKStorageBuffer}
    |
    |<----------------------|{VKCmdBuffer}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKSyncObject}
    |
    |<----------------------|VKResizing
    |
    |
    |(protected)
    |VKDrawSequence


    |<----------------------|{VKWindow}
    |
    |<----------------------|{VKInstance}
    |
    |<----------------------|{VKSurface}
    |
    |<----------------------|{VKLogDevice}
    |
    |<----------------------|{VKModelMgr}
    |
    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKBufferMgr}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|{VKCmdBuffer}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|{VKSyncObject}
    |
    |
    |(protected)
    |VKDeleteSequence
</pre>