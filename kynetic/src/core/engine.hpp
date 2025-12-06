//
// Created by kenny on 11/4/25.
//

#pragma once

namespace kynetic
{
class Engine
{
    std::unique_ptr<class Device> m_device;
    std::unique_ptr<class Input> m_input;
    std::unique_ptr<class ResourceManager> m_resource_manager;
    std::unique_ptr<class Scene> m_scene;
    std::unique_ptr<class Renderer> m_renderer;
    std::unique_ptr<class DebugRenderer> m_debug_renderer;

    bool is_shutting_down{false};

    std::function<void(float)> m_update_callback;
    std::function<void()> m_render_callback;

public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    [[nodiscard]] static Engine& get();

    [[nodiscard]] Device& device() const { return *m_device; }
    [[nodiscard]] Input& input() const { return *m_input; }
    [[nodiscard]] ResourceManager& resources() const { return *m_resource_manager; }
    [[nodiscard]] Scene& scene() const { return *m_scene; }

    [[nodiscard]] DebugRenderer& debug() const { return *m_debug_renderer; }

    void init();
    void shutdown();

    void update();

    void set_update_callback(const std::function<void(float)>& callback);
    void set_render_callback(const std::function<void()>& callback);
};
}  // namespace kynetic
