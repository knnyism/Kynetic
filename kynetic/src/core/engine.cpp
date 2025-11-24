//
// Created by kenny on 11/4/25.
//

#include "device.hpp"
#include "resource_manager.hpp"
#include "../rendering/scene.hpp"
#include "renderer.hpp"

#include "engine.hpp"

using namespace kynetic;

Engine* engine = nullptr;

Engine::Engine()
{
    KX_ASSERT_MSG(engine == nullptr, "There can only be one Engine object.");
    engine = this;

    m_device = std::make_unique<Device>();
    m_resource_manager = std::make_unique<ResourceManager>();
    m_scene = std::make_unique<Scene>();
    m_renderer = std::make_unique<Renderer>();
}

Engine::~Engine() { KX_ASSERT_MSG(is_shutting_down, "Engine was not shut down properly, did you forget to call ::shutdown?"); }

Engine& Engine::get() { return *engine; }

void Engine::init() {}

void Engine::shutdown()
{
    KX_ASSERT_MSG(!is_shutting_down, "Engine was already shutting down, ::shutdown can be called only once.");
    is_shutting_down = true;

    m_device->wait_idle();
}

void Engine::update()
{
    while (m_device->is_running())
    {
        m_device->update();
        m_scene->update();

        if (m_device->is_minimized())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (!m_device->begin_frame()) continue;
        m_renderer->render();
        m_device->end_frame();
    }
}