# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
    ENEnum
    :
    :
    |---------------------->|Config/ENEnvConfig
    |
    |---------------------->|Config/ENModelConfig


    |<----------------------|{VKDeviceMgr}
    |
    |<----------------------|{Utils/UserInput}
    |
    |<......................|ENEnvConfig
    |
    |
    |Control/ENGenericControl


    |<----------------------|{VKModelMgr}
    |
    |<----------------------|{VKCameraMgr}
    |
    |<----------------------|{UserInput}
    |
    |<......................|ENEnvConfig
    |
    |
    |Control/ENCameraControl


    |<----------------------|VKInitSequence
    |
    |<----------------------|VKDrawSequence
    |
    |<----------------------|VKDeleteSequence
    |
    |<----------------------|ENGenericControl
    |
    |<----------------------|ENCameraControl
    |
    |<......................|ENModelConfig
    |
    |
    |SandBox/ENApplication
    :
    :
    |main
</pre>