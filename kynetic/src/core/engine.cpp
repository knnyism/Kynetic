//
// Created by kenny on 11/4/25.
//

#include "device.hpp"
#include "input.hpp"
#include "resource_manager.hpp"
#include "scene.hpp"
#include "renderer.hpp"

#include "engine.hpp"

using namespace kynetic;

Engine* engine = nullptr;

Engine::Engine()
{
    KX_ASSERT_MSG(engine == nullptr, "There can only be one Engine object.");
    engine = this;

    m_device = std::make_unique<Device>();
    m_input = std::make_unique<Input>();
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
    auto time = std::chrono::high_resolution_clock::now();

    while (m_device->is_running())
    {
        auto ctime = std::chrono::high_resolution_clock::now();
        auto elapsed = ctime - time;
        const float delta_time = static_cast<float>(
            static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) / 1000000.0);

        m_device->update();
        m_update_callback(delta_time);
        m_input->update();

        m_scene->update();
        if (m_device->is_minimized())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else if (m_device->begin_frame())
        {
            m_renderer->render();
            m_device->end_frame();
        }

        time = ctime;
    }
}

void Engine::set_update_callback(const std::function<void(float)>& callback) { m_update_callback = callback; }