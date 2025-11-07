//
// Created by kennypc on 11/6/25.
//

#pragma once

#include "resource/base.hpp"

namespace kynetic
{

class ResourceManager
{
    friend class Engine;

    std::unordered_map<size_t, std::shared_ptr<Resource>> m_resources;

public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    template <typename T, typename... Args>
    std::shared_ptr<T> load(const std::filesystem::path& path, Args&&... args);

    template <typename T>
    std::shared_ptr<T> find(size_t id);
};

template <typename T, typename... Args>
std::shared_ptr<T> ResourceManager::load(const std::filesystem::path& path, Args&&... args)
{
    const auto id = std::hash<std::string>()(path.string());
    if (auto resource = find<T>(id)) return resource;

    m_resources[id] = std::make_shared<T>(path, std::forward<Args>(args)...);
    m_resources[id]->id = id;

    return std::dynamic_pointer_cast<T>(m_resources[id]);
}

template <typename T>
std::shared_ptr<T> ResourceManager::find(const size_t id)
{
    if (const auto it = m_resources.find(id); it != m_resources.end()) return std::dynamic_pointer_cast<T>(it->second);
    return std::shared_ptr<T>();
}
}  // namespace kynetic