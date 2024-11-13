# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

## Wrapper/
<pre>
                            |UIEnum
                            :
                            :
    |<......................|UIConfig
    :
    :
    |UIPrimitive


                            |UIEnum
                            :
                            :
    |<......................|UILogHelper
    :
    |<......................|UIConfig
    :
    :
    |UITree


    |<......................|UIConfig
    :
    :
    |UIOverlay


    |<......................|UIConfig
    :
    :
    |UIPlot
</pre>

## Gui/
<pre>
    |<----------------------|{VKWindow}
    |
    |<......................|UIConfig
    |
    |
    |(protected)
    |UIInput


    |<......................|UIConfig
    :
    :
    |UIUtil


    |<----------------------|{VKModelMatrix}
    |
    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKSceneMgr}
    |
    |<----------------------|UIPrimitive
    |
    |<----------------------|UITree
    |
    |<----------------------|UIPlot
    |
    |<----------------------|ENCamera
    |
    |<......................|ENLogHelper
    |
    |
    |(protected)
    |UIWindow


    |<----------------------|{VKRenderPassMgr}
    |
    |<----------------------|UIOverlay
    |
    |<----------------------|UIWindow
    |
    |
    |(protected)
    |UIImpl
</pre>