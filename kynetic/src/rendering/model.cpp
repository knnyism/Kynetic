//
// Created by kenny on 11/14/25.
//

#include "mesh.hpp"
#include "model.hpp"

#include "core/engine.hpp"
#include "core/resource_manager.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace kynetic;

Model::Model(const std::filesystem::path& path) : Resource(Type::Model, path.string())
{
    auto file = fastgltf::GltfDataBuffer::FromPath(path);
    constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices;

    fastgltf::Asset asset;
    fastgltf::Parser parser;

    auto loaded_asset = parser.loadGltf(file.get(), path.parent_path(), options);
    if (loaded_asset) asset = std::move(loaded_asset.get());

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<Primitive> primitives;

    std::function<void(const size_t, Node&)> traverse_node = [&](const size_t node_index, Node& parent_node)
    {
        indices.clear();
        vertices.clear();
        primitives.clear();

        auto& asset_node = asset.nodes[node_index];
        Node& node = parent_node.children.emplace_back();
        std::visit(fastgltf::visitor{[&](const fastgltf::TRS& trs)
                                     {
                                         node.translation = glm::make_vec3(&trs.translation[0]);
                                         node.rotation = glm::make_quat(&trs.rotation[0]);
                                         node.scale = glm::make_vec3(&trs.scale[0]);
                                     },
                                     [&](const fastgltf::math::fmat4x4&) { KX_ASSERT(false); }},
                   asset_node.transform);
        node.parent = &parent_node;

        if (asset_node.meshIndex.has_value())
        {
            auto& mesh = asset.meshes[asset_node.meshIndex.value()];
            auto name = mesh.name;

            for (auto& p : mesh.primitives)
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

            node.mesh = Engine::get().resources().load<Mesh>(path / name, indices, vertices, primitives);
        }

        for (const size_t child_index : asset_node.children) traverse_node(child_index, node);
    };

    for (const size_t node_index : asset.scenes[0].nodeIndices) traverse_node(node_index, m_root);
}