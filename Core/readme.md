# INHERITANCE GRAPH (and LOG INSTANCE IDs)
<i>Classes within {} are virtual inheritance</i>

<pre>
    VKConstants (N/A)
    |
    |------------------------------>|
    |                               |
    {VKInstanceHandle} (16)         VKWindow (11)
    |                               |
    |------------------>|<----------|
    |                   |
    |                   VKSurface (12)      VKConstants (N/A)
    |                   |                   |
    |                   |                   |
    {VKValidation} (15) VKQueue (5)         VKPhyDevice (18)
    |                   |                   |
    |------------------>|<------------------|
    |                   |
    VKInstance (9)      {VKLogDevice} (8)
                        |
                        |------------------------------------------------------------------>|
                        |                                                                   |
                        {VKSwapChain} (1)                                                   {VKGenericBuffer} (25)
                        |                                                                   |
    |<------------------|-------------------------------------->|-------------------------->|
    |                   |                                       |                           |
    VKImageView (20)    VKPhyDeviceHelper (7)                   |                           |
                                                                |                           |
                                                                |                           |
                        VKConstants (N/A)                       |                           VKUniformBuffer (26)
                        |                                       |                           |
                        |------------------>|                   |                           |
                        |                   |                   |                           |
                        VKUtils (10)        {VKVertexData} (21) {VKRenderPass} (3)          VKDescriptor (27)
                        |                   |                   |                           |
                        |------------------>|------------------>|<--------------------------|
                                                                |
                                                                VKPipeline (4)


    {VKLogDevice} (8)
    |
    |-------------------------------------->|
                                            |
    {VKGenericBuffer} (26)                  VKSyncObjects (24)
    |                                       |
    |-------------------------------------->|
                                            |
    {VKVertexData} (21)                     |
    |                                       |
    |-------------------------------------->|
                                            |
    {VKLogDevice} (8)                       |
    |                                       |
    |                                       |
    VKTransferCmdBuffer (23)                |                   VKImageView (20)            {VKRenderPass} (3)
    |                                       |                   |                           |
    |-------------------------------------->|                   |-------------------------->|
                                            |                                               |
    VKPipeline (4)                          VKVertexBuffer (22)                             {VKFrameBuffer} (17)
    |                                       |                                               |
    |-------------------------------------->|<----------------------------------------------|
                                            |                                               |
                                            {VKGraphicsCmdBuffer} (2)                       VKResizing (19)
                                            |                                               |
                                            |<----------------------------------------------|
                                            |
                                            VKDrawFrame (6)

    VKInstance (9)
    |
    |-------------------------------------->|
                                            |
    VKPhyDeviceHelper (7)                   |
    |                                       |
    |-------------------------------------->|
                                            |
    {VKGraphicsCmdBuffer} (2)               |
    |                                       |
    |-------------------------------------->|
                                            |
    VKDrawFrame (6)                         VKBase (13)
    |                                       |
    |-------------------------------------->|
                                            |
                                            VKRun (14)
                                            |
                                            |
                                            Application class (0)
</pre>