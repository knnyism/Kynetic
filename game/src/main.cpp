//
// Created by kennypc on 11/4/25.
//

#include "core/engine.hpp"

int main()
{
    kynetic::Engine engine;

    engine.init();
    engine.update();
    engine.shutdown();

    return 0;
}