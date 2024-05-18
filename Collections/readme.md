# Collections

## Import
<pre>
using namespace Collections;
</pre>

## Directory structure
<pre>
    Collections
    |-- Buffer             
    |-- Log          
</pre>

## Namespaces
<pre>
    <i>Collections</i>
        |-- <i>Buffer</i>
        |-- <i>Log</i>
</pre>

### Buffer
<pre>
    #include "path to Buffer/include/Buffer.h"

    // create a new buffer ('myBuffer' is a pointer to the buffer instance created)
    auto myBuffer = BUFFER_INIT (0,                                 // instance id
                                 Buffer::WITH_OVERFLOW,             // circular buffer type        
                                 int,                               // holds integer
                                 capacity);                         // buffer capacity

    // close this buffer using its instance id
    BUFFER_CLOSE (0);
</pre>

>*Buffer can be used as Queue or Stack using available methods*

### Log
<pre>
    #include "path to Log/include/Log.h"

    // create log instance
    auto myLog = LOG_INIT (0,                                       // instance id 
                           Log::INFO,                               // only log INFO level messages
                           Log::TO_FILE_IMMEDIATE |                 // dump log to file
                           Log::TO_FILE_BUFFER_CIRCULAR |           // dump log to circular buffered file with capacity
                           Log::TO_CONSOLE,                         // dump log to console
                           "path to Build/Log/"                     // file save location
                           5);                                      // circular buffered log file capacity

    // log an info level message
    LOG_INFO (myLog) << "Hello World! " 
                     << "This is a test message. " 
                     << 123 
                     << "," 
                     << 10.1010 
                     << std::endl;

    // close this log using its instance id 
    LOG_CLOSE (0);
</pre>