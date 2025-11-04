//
// Created by kennypc on 11/4/25.
//

#pragma once

namespace kynetic
{

class Device;

class Engine
{
    std::unique_ptr<Device> m_device;

public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    void init();
    void shutdown();

    void update();
};
}  // namespace kynetic
