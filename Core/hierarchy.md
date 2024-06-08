# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    VKConfig
    |
    |------------------------------>|
    |                               |
    {VKInstanceHandle}              VKWindow
    |                               |
    |------------------>|<----------|
    |                   |
    |                   VKSurface           LogHelper           VKConfig
    |                   |                   |                   |
    |                   |<------------------|                   |
    |                   |                                       |
    {VKValidation}      VKQueue                                 VKPhyDevice
    |                   |                                       |
    |------------------>|<--------------------------------------|
    |                   |
    VKInstance          {VKLogDevice}
                        |
                        |------------------------------------------------------------------>|
                        |                                                                   |
                        {VKSwapChain}                                                       {VKGenericBuffer}
                        |                                                                   |
    |<------------------|-------------------------------------->|-------------------------->|
    |                   |                                       |                           |
    VKImageView         VKPhyDeviceHelper                       |                           |
                                                                |                           |
                                                                |                           |
                        VKConfig                                |                           VKUniformBuffer
                        |                                       |                           |
                        |                                       |                           |
                        {VKVertexData}                          {VKRenderPass}              VKDescriptor
                        |                                       |                           |
                        |-------------------------------------->|<--------------------------|
                                                                |
                                                                VKPipeline


    {VKGenericBuffer}                       {VKVertexData}
    |                                       |
    |<--------------------------------------|
    |
    VKVertexBuffer
    |
    |-------------------------------------->|
                                            |
    {VKGenericBuffer}                       |
    |                                       |
    |                                       |
    VKIndexBuffer                           |                   VKImageView                 {VKRenderPass}
    |                                       |                   |                           |
    |-------------------------------------->|                   |-------------------------->|
                                            |                                               |
    VKPipeline                              |                                               {VKFrameBuffer}
    |                                       |                                               |
    |-------------------------------------->|<----------------------------------------------|
                                            |                                               |
    {VKLogDevice}                           {VKRecord}                                      VKResizing
    |                                       |                                               |
    |                                       |<----------------------------------------------|
    {VKSyncObjects}                         |
    |                                       |
    |-------------------------------------->|
                                            |
    {VKLogDevice}                           |
    |                                       |
    |                                       |
    {VKCmdBuffer}                           |
    |                                       |
    |-------------------------------------->|
                                            |
                                            VKGraphics


    {VKSyncObjects}                         {VKRecord}
    |                                       |
    |-------------------------------------->|
                                            |
    {VKCmdBuffer}                           |
    |                                       |
    |-------------------------------------->|
                                            |
                                            VKTransfer


    VKInstance
    |
    |-------------------------------------->|
                                            |
    VKPhyDeviceHelper                       |
    |                                       |
    |-------------------------------------->|
                                            |
    VKTransfer                              |
    |                                       |
    |-------------------------------------->|
                                            |
    VKGraphics                              |
    |                                       |
    |-------------------------------------->|
                                            |
                                            VKBase
                                            |
                                            |
                                            VKRun
                                            |
                                            |
                                            Application class
</pre>