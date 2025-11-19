//
// Created by kenny on 11/19/25.
//

#include "core/components.hpp"
#include "rendering/model.hpp"
#include "scene.hpp"

using namespace kynetic;

Scene::Scene()
{
    m_scene.observer<TransformComponent>("TransformDirty")
        .event(flecs::OnSet)
        .each([](TransformComponent& transform) { transform.is_dirty = true; });
}

void Scene::update()
{
    m_scene.query_builder<TransformComponent>().each(
        [](flecs::entity entity, TransformComponent& transform)
        {
            if (transform.is_dirty)
                entity.children<TransformComponent>(
                    [](flecs::entity child)
                    {
                        if (auto* child_transform = child.try_get_mut<TransformComponent>()) child_transform->is_dirty = true;
                    });
        });

    m_scene.query_builder<TransformComponent, TransformComponent>().term_at(1).parent().cascade().build().each(
        [](TransformComponent& transform, const TransformComponent& transform_parent)
        {
            if (transform.is_dirty)
            {
                transform.is_dirty = false;
                transform.transform = transform_parent.transform *
                                      make_transform_matrix(transform.translation, transform.rotation, transform.scale);
            }
        });
}

flecs::entity Scene::add_model(const std::shared_ptr<Model> model) const
{
    const flecs::entity root_entity = m_scene.entity();

    std::function<void(const Model::Node&, flecs::entity)> traverse_nodes =
        [&](const Model::Node& node, const flecs::entity parent)
    {
        const flecs::entity child_entity = m_scene.entity().child_of(parent).add<TransformComponent>();

        if (node.mesh)
        {
            child_entity.add<MeshComponent>();
            child_entity.set<MeshComponent>({node.mesh});
        }

        child_entity.set<TransformComponent>({.translation = node.translation, .rotation = node.rotation, .scale = node.scale});

        for (const auto& child : node.children) traverse_nodes(child, child_entity);
    };

    traverse_nodes(model->get_root_node(), root_entity);

    return root_entity;
}