//
// Created by kenny on 11/6/25.
//

#pragma once

namespace kynetic
{

class ResourceManager
{
    friend class Engine;
    friend class Renderer;

    std::unordered_map<size_t, std::shared_ptr<Resource>> m_resources;

    std::vector<std::shared_ptr<class Texture>> m_default_textures;

    AllocatedBuffer m_merged_vertex_buffer;
    AllocatedBuffer m_merged_index_buffer;

    VkDeviceAddress m_merged_vertex_buffer_address;

public:
    ResourceManager();
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    template <typename T, typename... Args>
    std::shared_ptr<T> load(const std::filesystem::path& path, Args&&... args);

    template <typename T>
    std::shared_ptr<T> find(const std::filesystem::path& path);

    template <typename T, typename Func>
    void for_each(Func&& func)
    {
        for (const auto& resource : m_resources | std::views::values)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(resource)) func(casted);
        }
    }

    void refresh_mesh_buffers();
};

template <typename T, typename... Args>
std::shared_ptr<T> ResourceManager::load(const std::filesystem::path& path, Args&&... args)
{
    if (auto resource = find<T>(path)) return resource;
    const auto id = std::hash<std::string>()(path.string());

    m_resources[id] = std::make_shared<T>(path, std::forward<Args>(args)...);
    m_resources[id]->id = id;

    return std::dynamic_pointer_cast<T>(m_resources[id]);
}

template <typename T>
std::shared_ptr<T> ResourceManager::find(const std::filesystem::path& path)
{
    const auto id = std::hash<std::string>()(path.string());
    if (const auto it = m_resources.find(id); it != m_resources.end()) return std::dynamic_pointer_cast<T>(it->second);
    return std::shared_ptr<T>();
}
}  // namespace kynetic