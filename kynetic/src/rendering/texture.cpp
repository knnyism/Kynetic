//
// Created by kenny on 11/17/25.
//

#include "descriptor.hpp"

#include "core/device.hpp"
#include "core/engine.hpp"

#include "texture.hpp"

using namespace kynetic;

void Texture::init(const VkSamplerCreateInfo& sampler_create_info)
{
    Device& device = Engine::get().device();

    vkCreateSampler(device.get(), &sampler_create_info, nullptr, &m_sampler);
}

Texture::Texture(const std::filesystem::path& path, const VkSamplerCreateInfo& sampler_create_info)
    : Resource(Type::Texture, path.string())
{
    // TODO
    init(sampler_create_info);
}

Texture::Texture(const std::filesystem::path& path,
                 void* data,
                 VkExtent3D extent,
                 VkFormat format,
                 VkImageUsageFlags usage_flags,
                 const VkSamplerCreateInfo& sampler_create_info)
    : Resource(Type::Texture, path.string())
{
    Device& device = Engine::get().device();
    m_image = device.create_image(data, extent, format, usage_flags);

    init(sampler_create_info);
}

Texture::~Texture()
{
    Device& device = Engine::get().device();

    device.destroy_image(m_image);
    vkDestroySampler(device.get(), m_sampler, nullptr);
}