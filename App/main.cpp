#include "../Core/VKRun.h"
#include "../Collections/Log/include/Log.h"

using namespace Collections;
using namespace Renderer;

int main (void) {
    /* Setup application logger
    */
    const size_t appInstanceId = 0;
    auto appLog = LOG_INIT (appInstanceId, 
                            Log::VERBOSE, 
                            Log::TO_CONSOLE | Log::TO_FILE_IMMEDIATE, 
                            "./Build/Log/");

    LOG_INFO (appLog) << "Hello World!" << std::endl;
    /* Create renderer object to use in application
    */
    VKRun* renderer = new VKRun;
    /* Check log manager and destroy app logger
     * LOG_MGR_DUMP;
    */
   
    /* Run renderer
    */
    renderer->runSequence();
    /* Delete renderer object
    */
    delete renderer;
    LOG_CLOSE (appInstanceId);
    return 0;
}