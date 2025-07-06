
#include "Engine/Core/App.h"

int main() {
    auto *app = new Kynetic::App();
    app->start();
    delete app;

    return 0;
}
