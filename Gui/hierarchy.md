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


    |<----------------------|{VKImageMgr}
    |
    |<----------------------|{VKSceneMgr}
    |
    |<----------------------|UIPrimitive
    |
    |<----------------------|UITree
    |
    |<----------------------|UIOverlay
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
    |<----------------------|UIWindow
    |
    |
    |(protected)
    |UIImpl
</pre>