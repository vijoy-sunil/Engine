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
    |---------------------->|{VKValidation}
    |                       |
    |                       |---------------------->|VKInstance
    |                       |
    |                       |---------------------->|VKLogDevice
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
                            |
                            |---------------------->|VKPhyDevice


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


    |{VKDeviceMgr}
    |
    |
    |Cmds/VKCmdBuffer


    |<----------------------|{VKBufferMgr}
    |
    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKPipelineMgr}
    |
    |
    |Cmds/VKCmds


    |<----------------------|{VKWindow}
    |
    |<----------------------|{VKInstance}
    |
    |<----------------------|{VKSurface}
    |
    |<----------------------|{VKLogDevice}
    |
    |<----------------------|VKSwapChainImage
    |
    |<----------------------|VKTextureImage
    |
    |<----------------------|VKDepthImage
    |
    |<----------------------|VKMultiSampleImage
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
    |<----------------------|VKPipelineLayout
    |
    |<----------------------|{VKTextureSampler}
    |
    |<----------------------|{VKDescriptor}
    |
    |
    |Scene/VKInitSequence


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
    |
    |Scene/VKDeleteSequence


    |<----------------------|VKInitSequence
    |
    |<----------------------|VKDeleteSequence
    |
    |
    |SandBox/RDApplication
    :
    :
    |main
</pre>