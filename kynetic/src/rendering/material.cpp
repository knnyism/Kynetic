//
// Created by kenny on 11/24/25.
//

#include "material.hpp"

using namespace kynetic;
Material::Material(const std::filesystem::path& path,
                   const std::shared_ptr<Texture>& albedo,
                   const std::shared_ptr<Texture>& normal,
                   const std::shared_ptr<Texture>& metal_roughness,
                   const std::shared_ptr<Texture>& emissive)
    : Resource(Type::Material, path.string()),
      m_albedo(albedo),
      m_normal(normal),
      m_metal_roughness(metal_roughness),
      m_emissive(emissive)
{
}