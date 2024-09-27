# Class/file hierarchy
<i>Classes within {} are virtual inheritance</i>

## Collection/
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