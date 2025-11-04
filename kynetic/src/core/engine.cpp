//
// Created by kennypc on 11/4/25.
//

#include "rendering/device.hpp"
#include "engine.hpp"

using namespace kynetic;

Engine::Engine() { m_device = std::make_unique<Device>(); }

Engine::~Engine() {}

void Engine::init() {}

void Engine::shutdown() {}

void Engine::update() {}