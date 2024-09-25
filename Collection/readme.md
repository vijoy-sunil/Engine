# Collection

## Import
<pre>
using namespace Collection;
</pre>

## Directory structure
<pre>
    Collection
    |-- Buffer
    |-- Log
</pre>

## Namespaces
<pre>
    <i>Collection</i>
    |-- <i>Admin</i>
    |-- <i>Buffer</i>
    |-- <i>Log</i>
</pre>

### Buffer
<pre>
    #include "path to Buffer/Buffer.h"

    // create a new buffer ('myBuffer' is a pointer to the buffer instance created)
    auto myBuffer = BUFFER_INIT (0,                                 // instance id
                                 Buffer::WITH_OVERFLOW,             // circular buffer type
                                 int,                               // holds integer
                                 capacity);                         // buffer capacity

    // push to buffer
    for (auto const& i: input)
        myBuffer->BUFFER_PUSH (i);

    // dump buffer contents
    myBuffer->BUFFER_DUMP;

    // close this buffer using its instance id
    BUFFER_CLOSE (0);
</pre>

>*Buffer can be used as Queue or Stack using available methods*

### Log
<pre>
    #include "path to Log/Log.h"

    // create log instance
    auto myLog = LOG_INIT (0,                                       // instance id
                           "path to save dir",                      // file save location
                           5);                                      // circular buffer log file capacity
    // add configs
    LOG_ADD_CONFIG (0, Log::INFO,    Log::TO_FILE_IMMEDIATE);
    LOG_ADD_CONFIG (0, Log::WARNING, Log::TO_CONSOLE | Log::TO_FILE_BUFFER_CIRCULAR);
    LOG_ADD_CONFIG (0, Log::ERROR,   Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE | Log::TO_FILE_BUFFER_CIRCULAR);

    // clear configs if you want to overwrite config
    LOG_CLEAR_CONFIG (0);
    LOG_ADD_CONFIG   (0, Log::INFO, Log::TO_CONSOLE);

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