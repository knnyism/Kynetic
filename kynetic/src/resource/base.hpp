#include <utility>

//
// Created by kennypc on 11/7/25.
//

#pragma once

namespace kynetic
{
struct Resource
{
    enum class Type
    {
        Shader,
        Texture,
        Mesh,
    };

    Type type;
    std::string path;

    size_t id{0};
    bool is_loaded{false};

    Resource(const Type type, std::string path) : type(type), path(std::move(path)) {}
    virtual ~Resource() = default;
};
}  // namespace kynetic