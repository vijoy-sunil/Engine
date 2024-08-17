#include "RDApplication.h"

int main (void) {
    SandBox::RDApplication app;
    app.createScene();
    app.runScene();
    app.deleteScene();
    return 0;
}