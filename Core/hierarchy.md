# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    |VKConfig, Log
    :
    :
    |{Device/VKDeviceMgr}
    |
    |---------------------->|VKWindow
    |
    |---------------------->|VKSurface
    |
    |                       |VKEnum
    |                       :
    |                       :
    |                       |Utils/LogHelper
    |                       :
    |                       :
    |---------------------->|VKQueue
    |                       |
    |                       |---------------------->|VKPhyDevice
    |
    |---------------------->|{VKValidation}
                            |
                            |---------------------->|VKInstance
                            |
                            |                       |{VKPhyDevice}
                            |                       |
                            |                       |
                            |---------------------->|VKLogDevice


    |VKConfig, Log
    :
    :
    |VKVertexData
    |
    |<......................|VKUniform
    |
    |{Model/VKModelMgr}
    |
    |
    |VKInstanceData


    |{VKPhyDevice}
    |
    |
    |{Image/VKImageMgr}
    |
    |---------------------->|VKSwapChainImage
    |
    |                       |{VKBufferMgr}
    |                       |
    |                       |
    |---------------------->|VKTextureImage
    |
    |---------------------->|VKDepthImage
    |
    |---------------------->|VKMultiSampleImage


    |{VKPhyDevice}
    |
    |
    |{Buffer/VKBufferMgr}
    |
    |---------------------->|VKVertexBuffer
    |
    |---------------------->|VKIndexBuffer
    |
    |---------------------->|VKUniformBuffer
    |
    |---------------------->|VKStorageBuffer


    |{VKDeviceMgr}
    |
    |
    |{RenderPass/VKRenderPassMgr}
    |
    |                       |{VKImageMgr}
    |                       |
    |                       |
    |---------------------->|VKAttachment
    |
    |---------------------->|VKSubPass
    |
    |---------------------->|VKFrameBuffer


    |{VKRenderPassMgr}
    |
    |
    |{Pipeline/VKPipelineMgr}
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
    |
    |Cmd/VKCmdBuffer


    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKBufferMgr}
    |
    |<----------------------|{VKPipelineMgr}
    |
    |
    |Cmd/VKCmd


    |<----------------------|{VKDeviceMgr}
    |
    |
    |Scene/VKCameraMgr


    |<----------------------|{VKDeviceMgr}
    |
    |<......................|LogHelper
    |
    |
    |Scene/VKSyncObject


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
    |
    |Scene/VKResizing


    |VKConfig, Log
    :
    :
    |{Scene/VKSceneMgr}     |{VKDeviceMgr}
    |                       |
    |                       |
    |---------------------->|VKTextureSampler
    |
    |                       |{VKPipelineMgr}
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
    |<----------------------|VKAttachment
    |
    |<----------------------|VKSubPass
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|VKVertexInput
    |
    |<----------------------|VKInputAssembly
    |
    |<----------------------|VKShaderStage
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
    |<----------------------|VKDescriptorSetLayout
    |
    |<----------------------|VKPushConstantRange
    |
    |<----------------------|VKPipelineLayout
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
    |
    |Scene/VKInitSequence


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
    |Scene/VKDrawSequence


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
    |Scene/VKDeleteSequence
</pre>