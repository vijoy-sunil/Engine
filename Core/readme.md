# INHERITANCE GRAPH (and LOG INSTANCE IDs)


<pre>
    VKInstanceHandle (16)           VKConstants (N/A)
    |                               |
    |                               |
    VKValidation (15)               |
    |                               |
    |-------------------------------|
    |                               |
    VKInstance (9)                  VKWindow (11)
    |                               |
    |-------------------|-----------|
                        |
                        VKSurface (12)
                        |
                        |
                        VKQueue (5)         VKPhyDevice (18)
                        |                   |
                        |-------------------|
                        |
                        VKLogDevice (8)
                        |
                        |
                        VKSwapChain (1)
                        |
                        |
                        VKPhyDeviceHelper (7)
                        |
                        |---------------------------------------|
                                                                |
                                                                |
                        VKUtils (10)        VKVertexData (21)   VKRenderPass (3) (virtual)
                        |                   |                   |
                        |                   |                   |-----------------------|
                        |                   |                   |                       |
                        |-------------------|-------------------|                       |
                                                                |                       |
                                                                VKPipeline (4)          VKImageView (20)
                                                                |                       |
                                                                |                       |
                                                                VKVertexBuffer (22)     VKFrameBuffer (17)
                                                                |                       |
                                                                |                       |
                                                                |                       VKResizing (19)
                                                                |                       |
                                                                |-----------------------|
                                                                |
                                                                |
                                                                VKCmdBuffer (2)
                                                                |
                                                                |
                                                                VKDrawFrame (6)
                                                                |
                                                                |
                                                                VKBase (13)
                                                                |
                                                                |
                                                                VKRun (14)
                                                                |
                                                                |
                                                                Application class (0)
</pre>