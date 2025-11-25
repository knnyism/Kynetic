//
// Created by kenny on 11/17/25.
//

#pragma once
#include "pch.hpp"

namespace kynetic
{

class Texture : public Resource
{
    friend class ResourceManager;
    friend class Renderer;

    uint32_t m_handle{0};

    AllocatedImage m_image;
    VkSampler m_sampler;

    void init(const VkSamplerCreateInfo& sampler_create_info);

public:
    Texture(const std::filesystem::path& path, const VkSamplerCreateInfo& sampler_create_info);
    Texture(const std::filesystem::path& path,
            void* data,
            VkExtent3D extent,
            VkFormat format,
            VkImageUsageFlags usage_flags,
            const VkSamplerCreateInfo& sampler_create_info);
    ~Texture() override;
};

}  // namespace kynetic
