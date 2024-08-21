#include "ENApplication.h"

int main (void) {
    SandBox::ENApplication app;
    app.createScene();
    app.runScene();
    app.deleteScene();
    return 0;
}