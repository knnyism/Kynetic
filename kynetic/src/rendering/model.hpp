//
// Created by kenny on 11/14/25.
//

#pragma once

namespace kynetic
{
class Model : public Resource
{
    std::vector<std::shared_ptr<class Mesh>> m_meshes;

public:
    Model(const std::filesystem::path& path);

    [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& get_meshes() { return m_meshes; }
};
}  // namespace kynetic