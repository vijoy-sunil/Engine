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
    |Cmds/VKCmdBuffer


    |<----------------------|{VKBufferMgr}
    |
    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKPipelineMgr}
    |
    |
    |Cmds/VKCmds


    |<----------------------|{VKDeviceMgr}
    |
    |
    |Scene/VKCameraMgr


    |<----------------------|{VKDeviceMgr}
    |
    |<......................|LogHelper
    |
    |
    |Scene/VKSyncObjects


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
    |<----------------------|{VKModelMatrix}
    |
    |<----------------------|{VKCmdBuffer}
    |
    |<----------------------|{VKCmds}
    |
    |<......................|VKUniforms
    |
    |<......................|VKTransform
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKSyncObjects}
    |
    |<----------------------|{VKDrawSequence}
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
    |<----------------------|{VKCmds}
    |
    |<......................|VKUniforms
    |
    |<......................|VKTransform
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{VKSyncObjects}
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
    |<----------------------|{VKSyncObjects}
    |
    |<----------------------|{VKDrawSequence}
    |
    |
    |Scene/VKDeleteSequence


    |<----------------------|VKInitSequence
    |
    |<----------------------|{VKDrawSequence}
    |
    |<----------------------|VKDeleteSequence
    |
    |
    |SandBox/RDApplication
    :
    :
    |main
</pre>