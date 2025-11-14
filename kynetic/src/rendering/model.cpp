//
// Created by kennypc on 11/14/25.
//

#include "mesh.hpp"
#include "model.hpp"

#include "core/engine.hpp"
#include "core/resource_manager.hpp"

using namespace kynetic;

Model::Model(const std::filesystem::path& path) : Resource(Type::Model, path.string())
{
    auto file = fastgltf::GltfDataBuffer::FromPath(path);
    constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices;

    fastgltf::Asset asset;
    fastgltf::Parser parser;

    auto loaded_asset = parser.loadGltf(file.get(), path.parent_path(), options);
    if (loaded_asset)
    {
        asset = std::move(loaded_asset.get());
    }

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<Primitive> primitives;
    for (fastgltf::Mesh& mesh : asset.meshes)
    {
        auto name = mesh.name;

        indices.clear();
        vertices.clear();
        primitives.clear();

        for (auto&& p : mesh.primitives)
        {
            Primitive primitive;

            primitive.first = static_cast<uint32_t>(indices.size());
            primitive.count = static_cast<uint32_t>(asset.accessors[p.indicesAccessor.value()].count);

            size_t initial_vtx = vertices.size();

            {
                fastgltf::Accessor& indexaccessor = asset.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(asset,
                                                         indexaccessor,
                                                         [&](const std::uint32_t idx)
                                                         { indices.push_back(static_cast<uint32_t>(idx + initial_vtx)); });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = asset.accessors[p.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                              posAccessor,
                                                              [&](glm::vec3 v, size_t index)
                                                              {
                                                                  Vertex newvtx;
                                                                  newvtx.position = v;
                                                                  newvtx.normal = {1, 0, 0};
                                                                  newvtx.color = glm::vec4{1.f};
                                                                  newvtx.uv_x = 0;
                                                                  newvtx.uv_y = 0;
                                                                  vertices[initial_vtx + index] = newvtx;
                                                              });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                              asset.accessors[normals->accessorIndex],
                                                              [&](glm::vec3 v, size_t index)
                                                              { vertices[initial_vtx + index].normal = v; });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(asset,
                                                              asset.accessors[uv->accessorIndex],
                                                              [&](glm::vec2 v, size_t index)
                                                              {
                                                                  vertices[initial_vtx + index].uv_x = v.x;
                                                                  vertices[initial_vtx + index].uv_y = v.y;
                                                              });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                              asset.accessors[colors->accessorIndex],
                                                              [&](glm::vec4 v, size_t index)
                                                              { vertices[initial_vtx + index].color = v; });
            }

            primitives.push_back(primitive);
        }

        constexpr bool OVERRIDE_COLORS = true;
        if (OVERRIDE_COLORS)
        {
            for (Vertex& vtx : vertices)
            {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }

        m_meshes.push_back(Engine::get().resources().load<Mesh>(path / name, indices, vertices, primitives));
    }
}