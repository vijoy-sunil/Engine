# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    |<----------------------|{VKWindow}
    |
    |<......................|UIConfig
    |
    |
    |(protected)
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