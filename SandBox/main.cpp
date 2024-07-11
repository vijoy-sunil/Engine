#include "RDApplication.h"

int main (void) {
    Renderer::RDApplication app;
    app.createScene();
    app.runScene();
    app.deleteScene();
    return 0;
}