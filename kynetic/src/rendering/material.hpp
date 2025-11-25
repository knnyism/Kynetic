//
// Created by kenny on 11/24/25.
//

#pragma once

namespace kynetic
{

class Texture;

class Material : public Resource
{
    friend class ResourceManager;

    uint32_t m_handle{0};

    std::shared_ptr<Texture> m_albedo;
    std::shared_ptr<Texture> m_normal;
    std::shared_ptr<Texture> m_metal_roughness;
    std::shared_ptr<Texture> m_emissive;

public:
    Material(const std::filesystem::path& path,
             const std::shared_ptr<Texture>& albedo,
             const std::shared_ptr<Texture>& normal,
             const std::shared_ptr<Texture>& metal_roughness,
             const std::shared_ptr<Texture>& emissive);

    [[nodiscard]] uint32_t get_handle() const { return m_handle; }
};

}  // namespace kynetic
