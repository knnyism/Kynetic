//
// Created by kenny on 11/6/25.
//

#include "rendering/texture.hpp"
#include "resource_manager.hpp"

using namespace kynetic;
ResourceManager::ResourceManager()
{
    VkSamplerCreateInfo nearest_sampler{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
    };

    uint32_t white = glm::packUnorm4x8(glm::vec4(1.f, 1.f, 1.f, 1.f));
    m_default_textures.push_back(load<Texture>("dev/white",
                                               &white,
                                               VkExtent3D{1, 1, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.f));
    m_default_textures.push_back(load<Texture>("dev/grey",
                                               &grey,
                                               VkExtent3D{1, 1, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));
    uint32_t black = glm::packUnorm4x8(glm::vec4(0.f, 0.f, 0.f, 0.f));
    m_default_textures.push_back(load<Texture>("dev/black",
                                               &black,
                                               VkExtent3D{1, 1, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));

    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.f, 0.f, 1.f, 1.f));

    std::array<uint32_t, 16 * 16> missing;
    for (uint32_t x = 0; x < 16; x++)
        for (uint32_t y = 0; y < 16; y++) missing[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;

    m_default_textures.push_back(load<Texture>("dev/missing",
                                               &missing,
                                               VkExtent3D{16, 16, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));
}