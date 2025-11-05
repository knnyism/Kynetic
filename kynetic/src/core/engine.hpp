//
// Created by kennypc on 11/4/25.
//

#pragma once

namespace kynetic
{
class Engine
{
    std::unique_ptr<class Device> m_device;
    std::unique_ptr<class Renderer> m_renderer;

    bool is_shutting_down{false};

public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    [[nodiscard]] static Engine& get();
    [[nodiscard]] Device& get_device() const { return *m_device; }

    void init();
    void shutdown();

    void update();
};
}  // namespace kynetic
