
#include "Engine/Core/App.h"

int main() {
    glfwInit();

    auto *app = new Kynetic::App();
    app->Start();
    delete app;

    glfwTerminate();

    return 0;
}
