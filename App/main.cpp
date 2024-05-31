#include "RDApp.h"

using namespace Renderer;

int main (void) {
    /* Create renderer object
    */
    RDApp* rendererApp = new RDApp;   
    /* Run renderer
    */
    rendererApp-> runApp();
    /* Delete renderer object
    */
    delete rendererApp;
    return 0;
}