//
// Created by kenny on 11/19/25.
//

#pragma once

namespace kynetic
{

class Scene
{
    flecs::world m_scene;

public:
    Scene();

    void update();

    flecs::entity add_model(const std::shared_ptr<class Model> model) const;
    flecs::world& get() { return m_scene; }
};

}  // namespace kynetic
