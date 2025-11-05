//
// Created by kennypc on 11/4/25.
//

#include "device.hpp"
#include "renderer.hpp"

#include "engine.hpp"

using namespace kynetic;

Engine* engine = nullptr;

Engine::Engine()
{
    assert(engine == nullptr);
    engine = this;

    m_device = std::make_unique<Device>();
    m_renderer = std::make_unique<Renderer>();
}

Engine::~Engine()
{
    assert(is_shutting_down);
    engine = nullptr;
}

Engine& Engine::get() { return *engine; }

void Engine::init() {}

void Engine::shutdown()
{
    assert(!is_shutting_down);
    is_shutting_down = true;

    m_device->wait_idle();
}

void Engine::update()
{
    while (m_device->is_running())
    {
        m_device->update();

        if (m_device->is_minimized())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        m_device->begin_frame();
        m_renderer->render();
        m_device->end_frame();
    }
}