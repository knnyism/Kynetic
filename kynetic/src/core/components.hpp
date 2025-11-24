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
    glm::vec3 translation{0.f};                      // Local space
    glm::quat rotation{glm::identity<glm::quat>()};  // Local space
    glm::vec3 scale{1.f};                            // Local space

    glm::mat4 transform{1.f};  // Global space

    bool is_dirty{true};
};

struct MainCameraTag
{
};

struct CameraComponent
{
    float aspect;
    float near_plane;
    float far_plane;
    float fovy;
};

}  // namespace kynetic