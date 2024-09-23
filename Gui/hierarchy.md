# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    |<----------------------|{VKWindow}
    |                       |(protected)
    |
    |<......................|UIConfig
    |
    |
    |UIInput


    |<----------------------|{VKRenderPassMgr}
    |
    |<----------------------|{VKSceneMgr}
    |
    |<----------------------|UIInput
    |
    |
    |(protected)
    |UIImpl
</pre>