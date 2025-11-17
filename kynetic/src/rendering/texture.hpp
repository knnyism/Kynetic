//
// Created by kenny on 11/17/25.
//

#pragma once
#include "pch.hpp"

namespace kynetic
{

class Texture : public Resource
{
public:
    AllocatedImage m_image;
    VkSampler m_sampler;
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
