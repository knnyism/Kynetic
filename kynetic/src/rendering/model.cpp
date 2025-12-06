//
// Created by kenny on 11/14/25.
//

#include "mesh.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "model.hpp"

#include "core/engine.hpp"
#include "core/resource_manager.hpp"

#include "stb_image.h"

KX_DISABLE_WARNING_PUSH
KX_DISABLE_WARNING_CONVERSION
KX_DISABLE_WARNING_SIGNED_UNSIGNED_ASSIGNMENT_MISMATCH
KX_DISABLE_WARNING_HIDES_LOCAL_DECLARATION
#include "mikktspace.h"
KX_DISABLE_WARNING_POP

using namespace kynetic;

VkFilter extract_filter(fastgltf::Filter filter)
{
    switch (filter)
    {
        // nearest samplers
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;

        // linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
    switch (filter)
    {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

Model::Model(const std::filesystem::path& path) : Resource(Type::Model, path.string())
{
    constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages |
                             fastgltf::Options::DecomposeNodeMatrices;
    auto file = fastgltf::GltfDataBuffer::FromPath(path);

    fastgltf::Asset asset;
    fastgltf::Parser parser;

    fastgltf::Expected<fastgltf::Asset> loaded_asset = parser.loadGltf(file.get(), path.parent_path(), options);
    KX_ASSERT_MSG(loaded_asset.error() == fastgltf::Error::None, "failed to load gltf ({} - {}): {}", path.parent_path().string(), path.string(), fastgltf::getErrorMessage(loaded_asset.error()));

    asset = std::move(loaded_asset.get());

    auto validation = fastgltf::validate(asset);
    KX_ASSERT_MSG(validation == fastgltf::Error::None, "glTF validation failed: {}", fastgltf::getErrorMessage(validation));

    std::vector<uint32_t> indices;
    std::vector<glm::vec4> positions;
    std::vector<Vertex> vertices;

    auto load_texture = [&](const fastgltf::TextureInfo& texture_info, VkFormat image_format) -> std::shared_ptr<Texture>
    {
        auto& texture_asset = asset.textures[texture_info.textureIndex];
        const std::filesystem::path texture_path = path / "texture" / std::to_string(texture_info.textureIndex);
        std::shared_ptr<Texture> texture = Engine::get().resources().find<Texture>(texture_path);
        if (texture) return texture;

        fastgltf::Image& image = asset.images[texture_asset.imageIndex.value()];
        fastgltf::Sampler& sampler = asset.samplers[texture_asset.samplerIndex.value()];

        VkSamplerCreateInfo sampler_create_info{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
        sampler_create_info.minLod = 0;

        sampler_create_info.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampler_create_info.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampler_create_info.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        int width, height, nrChannels;
        unsigned char* data;

        std::visit(
            fastgltf::visitor{
                [](auto&) {},
                [&](fastgltf::sources::URI& file_path)
                {
                    KX_ASSERT(file_path.fileByteOffset == 0);  // We don't support offsets with stbi.
                    KX_ASSERT(file_path.uri.isLocalPath());    // We're only capable of loading local files.

                    const std::string path(file_path.uri.path());  // Thanks C++.
                    data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                },
                [&](fastgltf::sources::Array& vector)
                {
                    data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                                                 static_cast<int>(vector.bytes.size()),
                                                 &width,
                                                 &height,
                                                 &nrChannels,
                                                 4);
                },
                [&](fastgltf::sources::BufferView& view)
                {
                    auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                    auto& buffer = asset.buffers[bufferView.bufferIndex];
                    // Yes, we've already loaded every buffer into some GL buffer. However, with GL it's simpler
                    // to just copy the buffer data again for the texture. Besides, this is just an example.
                    std::visit(
                        fastgltf::visitor{
                            // We only care about VectorWithMime here, because we specify LoadExternalBuffers, meaning
                            // all buffers are already loaded into a vector.
                            [](auto&) {},
                            [&](fastgltf::sources::Array& vector)
                            {
                                data = stbi_load_from_memory(
                                    reinterpret_cast<const stbi_uc*>(vector.bytes.data() + bufferView.byteOffset),
                                    static_cast<int>(bufferView.byteLength),
                                    &width,
                                    &height,
                                    &nrChannels,
                                    4);
                            }},
                        buffer.data);
                },
            },
            image.data);

        VkExtent3D extent;
        extent.width = static_cast<uint32_t>(width);
        extent.height = static_cast<uint32_t>(height);
        extent.depth = 1;

        texture = Engine::get().resources().load<Texture>(texture_path,
                                                          data,
                                                          extent,
                                                          image_format,
                                                          VK_IMAGE_USAGE_SAMPLED_BIT,
                                                          sampler_create_info);

        stbi_image_free(data);

        return texture;
    };

    auto load_material = [&](size_t material_index) -> std::shared_ptr<Material>
    {
        const std::filesystem::path material_path = path / "material" / std::to_string(material_index);
        const fastgltf::Material& material_asset = asset.materials[material_index];
        std::shared_ptr<Material> material = Engine::get().resources().find<Material>(material_path);
        if (material) return material;

        std::shared_ptr<Texture> albedo =
            material_asset.pbrData.baseColorTexture.has_value()
                ? load_texture(material_asset.pbrData.baseColorTexture.value(), VK_FORMAT_R8G8B8A8_UNORM)
                : Engine::get().resources().find<Texture>("dev/white");

        std::shared_ptr<Texture> normal = material_asset.normalTexture.has_value()
                                              ? load_texture(material_asset.normalTexture.value(), VK_FORMAT_R8G8B8A8_UNORM)
                                              : Engine::get().resources().find<Texture>("dev/normal");

        std::shared_ptr<Texture> metal_roughness =
            material_asset.pbrData.metallicRoughnessTexture.has_value()
                ? load_texture(material_asset.pbrData.metallicRoughnessTexture.value(), VK_FORMAT_R8G8B8A8_UNORM)
                : Engine::get().resources().find<Texture>("dev/black");

        std::shared_ptr<Texture> emissive = material_asset.emissiveTexture.has_value()
                                                ? load_texture(material_asset.emissiveTexture.value(), VK_FORMAT_R8G8B8A8_UNORM)
                                                : Engine::get().resources().find<Texture>("dev/black");

        return Engine::get().resources().load<Material>(material_path, albedo, normal, metal_roughness, emissive);
    };

    uint32_t mesh_index = 0;
    std::function<void(size_t, Node&)> traverse_node = [&](const size_t node_index, Node& parent_node)
    {
        auto& asset_node = asset.nodes[node_index];
        Node& node = parent_node.children.emplace_back();
        std::visit(fastgltf::visitor{[&](const fastgltf::TRS& trs)
                                     {
                                         node.translation = glm::make_vec3(trs.translation.data());
                                         node.rotation = glm::make_quat(trs.rotation.data());
                                         node.scale = glm::make_vec3(trs.scale.data());
                                     },
                                     [&](const fastgltf::math::fmat4x4&) { KX_ASSERT(false); }},
                   asset_node.transform);
        node.parent = &parent_node;

        if (asset_node.meshIndex.has_value())
        {
            auto& mesh = asset.meshes[asset_node.meshIndex.value()];
            auto name = mesh.name;

            for (size_t prim_index = 0; prim_index < mesh.primitives.size(); ++prim_index)
            {
                auto& p = mesh.primitives[prim_index];

                indices.clear();
                positions.clear();
                vertices.clear();

                size_t initial_vtx = vertices.size();

                // load indices
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
                    positions.resize(positions.size() + posAccessor.count);
                    vertices.resize(vertices.size() + posAccessor.count);

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                                  posAccessor,
                                                                  [&](glm::vec3 v, size_t index)
                                                                  {
                                                                      Vertex newvtx;
                                                                      newvtx.normal = {1, 0, 0};
                                                                      newvtx.color = glm::vec4{1.f};
                                                                      newvtx.uv_x = 0;
                                                                      newvtx.uv_y = 0;
                                                                      positions[initial_vtx + index] = glm::vec4(v, 0.f);
                                                                      vertices[initial_vtx + index] = newvtx;
                                                                  });
                }

                // load vertex normals
                auto* normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset,
                                                                  asset.accessors[normals->accessorIndex],
                                                                  [&](glm::vec3 v, size_t index)
                                                                  { vertices[initial_vtx + index].normal = v; });
                }

                // load UVs
                auto* uv = p.findAttribute("TEXCOORD_0");
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

                // load tangents or generate them
                auto* tangents = p.findAttribute("TANGENT");
                if (tangents != p.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                                  asset.accessors[tangents->accessorIndex],
                                                                  [&](glm::vec4 v, size_t index)
                                                                  { vertices[initial_vtx + index].tangent = v; });
                }
                else
                {
                    struct Context
                    {
                        std::vector<uint32_t>* indices;
                        std::vector<glm::vec4>* positions;
                        std::vector<Vertex>* vertices;
                    } userContext;
                    userContext.indices = &indices;
                    userContext.positions = &positions;
                    userContext.vertices = &vertices;

                    SMikkTSpaceInterface mikkTSpaceInterface;
                    mikkTSpaceInterface.m_getNumFaces = [](SMikkTSpaceContext const* pContext) -> int
                    {
                        const auto* context = static_cast<Context*>(pContext->m_pUserData);
                        return static_cast<int>(context->indices->size()) / 3;
                    };
                    mikkTSpaceInterface.m_getNumVerticesOfFace = [](SMikkTSpaceContext const*, int) -> int { return 3; };
                    mikkTSpaceInterface.m_getPosition =
                        [](SMikkTSpaceContext const* pContext, float* fvPosOut, int iFace, int iVert)
                    {
                        const auto* context = static_cast<Context*>(pContext->m_pUserData);
                        const auto& vertexIndex = context->indices->at(static_cast<size_t>(iFace * 3 + iVert));
                        const auto& position = context->positions->at(vertexIndex);

                        fvPosOut[0] = position.x;
                        fvPosOut[1] = position.y;
                        fvPosOut[2] = position.z;
                    };
                    mikkTSpaceInterface.m_getNormal =
                        [](SMikkTSpaceContext const* pContext, float* fvNormOut, int iFace, int iVert)
                    {
                        const auto* context = static_cast<Context*>(pContext->m_pUserData);
                        const auto& vertexIndex = context->indices->at(static_cast<size_t>(iFace * 3 + iVert));
                        const auto& vertex = context->vertices->at(vertexIndex);

                        fvNormOut[0] = vertex.normal.x;
                        fvNormOut[1] = vertex.normal.y;
                        fvNormOut[2] = vertex.normal.z;
                    };
                    mikkTSpaceInterface.m_getTexCoord =
                        [](SMikkTSpaceContext const* pContext, float* fvTexcOut, int iFace, int iVert)
                    {
                        const auto* context = static_cast<Context*>(pContext->m_pUserData);
                        const auto vertexIndex = context->indices->at(static_cast<size_t>(iFace * 3 + iVert));
                        const auto& vertex = context->vertices->at(vertexIndex);

                        fvTexcOut[0] = vertex.uv_x;
                        fvTexcOut[1] = vertex.uv_y;
                    };
                    mikkTSpaceInterface.m_setTSpaceBasic =
                        [](SMikkTSpaceContext const* pContext, float const* fvTangent, float fSign, int iFace, int iVert)
                    {
                        auto* context = static_cast<Context*>(pContext->m_pUserData);
                        const auto vertexIndex = context->indices->at(static_cast<size_t>(iFace * 3 + iVert));
                        auto& vertex = context->vertices->at(vertexIndex);

                        vertex.tangent.x = fvTangent[0];
                        vertex.tangent.y = fvTangent[1];
                        vertex.tangent.z = fvTangent[2];
                        vertex.tangent.w = fSign;
                    };
                    mikkTSpaceInterface.m_setTSpace = nullptr;

                    SMikkTSpaceContext mikkTSpaceContext;
                    mikkTSpaceContext.m_pInterface = &mikkTSpaceInterface;
                    mikkTSpaceContext.m_pUserData = &userContext;

                    genTangSpaceDefault(&mikkTSpaceContext);
                }

                // load vertex colors
                auto* colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(asset,
                                                                  asset.accessors[colors->accessorIndex],
                                                                  [&](glm::vec4 v, size_t index)
                                                                  { vertices[initial_vtx + index].color = v; });
                }

                node.meshes.push_back(Engine::get().resources().load<Mesh>(path / "mesh" / name / std::to_string(prim_index),
                                                                           mesh_index++,
                                                                           indices,
                                                                           positions,
                                                                           vertices,
                                                                           load_material(p.materialIndex.value())));
            }
        }

        for (const size_t child_index : asset_node.children) traverse_node(child_index, node);
    };

    for (const size_t node_index : asset.scenes[0].nodeIndices) traverse_node(node_index, m_root);
}