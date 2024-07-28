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


    |VKConfig, Log
    :
    :
    |VKVertexData
    |
    |
    |{Model/VKModelMgr}
    |
    |                       |{VKDeviceMgr}
    |                       |
    |                       |
    |---------------------->|VKTextureSampler
    |
    |                       |{VKPipelineMgr}
    |                       |
    |                       |
    |---------------------->|VKDescriptor
    |
    |---------------------->|VKModelMatrix


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


    |<----------------------|{VKModelMgr}
    |
    |<----------------------|{VKSwapChainImage}
    |
    |<----------------------|{VKDepthImage}
    |
    |<----------------------|{VKMultiSampleImage}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |
    |Scene/VKResizing


    |<......................|VKUniform
    |
    |<......................|VKTransform
    |
    |<......................|VKConfig
    |
    |<......................|Log
    |
    |
    |Scene/VKHandOFF


    |<----------------------|{VKWindow}
    |
    |<----------------------|{VKInstance}
    |
    |<----------------------|{VKSurface}
    |
    |<----------------------|{VKLogDevice}
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
    |<----------------------|{VKUniformBuffer}
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
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|{VKModelMatrix}
    |
    |<----------------------|{VKCmdBuffer}
    |
    |<----------------------|{VKCmd}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKSyncObject}
    |
    |<----------------------|{VKHandOff}
    |
    |
    |Scene/VKInitSequence


    |<----------------------|{VKWindow}
    |
    |<----------------------|{VKUniformBuffer}
    |
    |<----------------------|{VKModelMatrix}
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
    |<----------------------|{VKHandOff}
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
    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKBufferMgr}
    |
    |<----------------------|{VKFrameBuffer}
    |
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |<----------------------|{VKCmdBuffer}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKSyncObject}
    |
    |<----------------------|{VKHandOff}
    |
    |
    |Scene/VKDeleteSequence


    |<----------------------|VKInitSequence
    |
    |<----------------------|VKDrawSequence
    |
    |<----------------------|VKDeleteSequence
    |
    |
    |SandBox/RDApplication
    :
    :
    |main
</pre>