//
// Created by kenny on 11/19/25.
//

#pragma once

namespace kynetic
{

struct MeshComponent
{
    std::vector<std::shared_ptr<class Mesh>> meshes;
};

struct TransformComponent
{
    glm::vec3 translation;  // Local space
    glm::quat rotation;     // Local space
    glm::vec3 scale;        // Local space

    glm::mat4 transform{1.f};  // Global space

    bool is_dirty{true};
};
}  // namespace kynetic