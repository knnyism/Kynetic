//
// Created by kenny on 11/14/25.
//

#pragma once

namespace kynetic
{
class Model : public Resource
{
public:
    struct Node
    {
        glm::vec3 translation{0.0f, 0.0f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};

        Node* parent{nullptr};
        std::vector<Node> children;

        std::shared_ptr<class Mesh> mesh;
    };

private:
    Node m_root{};

public:
    Model(const std::filesystem::path& path);

    [[nodiscard]] const Node& get_root_node() { return m_root; }
};
}  // namespace kynetic