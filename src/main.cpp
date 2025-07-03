
#include "Engine/Core/App.h"

int main() {
    auto *app = new Kynetic::App();

    app->Start();

    delete app;

    return 0;
}
