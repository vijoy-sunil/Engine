# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

<pre>
                            |DumpFormatting
                            :
                            :
    |InstanceMgr            |InstanceMgr                            |NonTemplateBase
    |(public)               :                                       |(public)
    |                       :                                       |
    |                       |BufferImpl     |<----------------------|
    |                       :                                       |
    |                       :                                       |
    |---------------------->|BufferMgr                              |
    |                       :                                       |
    |                       :                                       |
    |                       |Buffer                                 |
    |                       :                                       |
    |                       :                                       |
    |                       |Record         |<----------------------|
    |                       :
    |                       :
    |---------------------->|RecordMgr
                            :
                            :
                            |Log
</pre>