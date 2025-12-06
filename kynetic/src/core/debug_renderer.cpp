//
// Debug Renderer Implementation
//

#include "debug_renderer.hpp"
#include "device.hpp"
#include "engine.hpp"
#include "scene.hpp"
#include "resource_manager.hpp"

#include "rendering/shader.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/descriptor.hpp"

using namespace kynetic;

DebugRenderer::DebugRenderer()
{
    Device& device = Engine::get().device();

    m_debug_line_shader = Engine::get().resources().load<Shader>("assets/shaders/debug_line.slang");
    m_debug_solid_shader = Engine::get().resources().load<Shader>("assets/shaders/debug_solid.slang");

    // Line pipeline
    m_debug_line_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                           .set_input_topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                                                           .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                           .set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT)
                                                           .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                                           .set_depth_format(VK_FORMAT_D32_SFLOAT)
                                                           .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                                           .set_multisampling_none()
                                                           .disable_blending()
                                                           .set_shader(m_debug_line_shader)
                                                           .build(device));

    // Solid pipeline with alpha blending
    m_debug_solid_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                            .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                            .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                            .set_color_attachment_format(VK_FORMAT_R16G16B16A16_SFLOAT)
                                                            .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                                            .set_depth_format(VK_FORMAT_D32_SFLOAT)
                                                            .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                                            .set_multisampling_none()
                                                            .enable_blending_alpha()
                                                            .set_shader(m_debug_solid_shader)
                                                            .build(device));
}

DebugRenderer::~DebugRenderer()
{
    Device& device = Engine::get().device();

    if (m_line_vertex_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_line_vertex_buffer);
    if (m_triangle_vertex_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_triangle_vertex_buffer);
}

void DebugRenderer::clear()
{
    m_line_vertices.clear();
    m_triangle_vertices.clear();
    m_buffers_dirty = true;
}

void DebugRenderer::add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    m_line_vertices.push_back({start, color});
    m_line_vertices.push_back({end, color});
    m_buffers_dirty = true;
}

void DebugRenderer::add_sphere(const glm::vec3& center, float radius, const glm::vec4& color)
{
    generate_sphere_mesh(center, radius, color);
    m_buffers_dirty = true;
}

void DebugRenderer::add_cone(const glm::vec3& apex, const glm::vec3& axis, float height, float angle, const glm::vec4& color)
{
    generate_cone_mesh(apex, axis, height, angle, color);
    m_buffers_dirty = true;
}

void DebugRenderer::add_frustum(const glm::mat4& view_proj, const glm::vec4& color)
{
    glm::mat4 inv_vp = glm::inverse(view_proj);
    generate_frustum_lines(inv_vp, color);
    m_buffers_dirty = true;
}

void DebugRenderer::generate_sphere_mesh(const glm::vec3& center, float radius, const glm::vec4& color, int segments, int rings)
{
    const float pi = glm::pi<float>();

    for (int ring = 0; ring < rings; ++ring)
    {
        float theta1 = (static_cast<float>(ring) / static_cast<float>(rings)) * pi;
        float theta2 = (static_cast<float>(ring + 1) / static_cast<float>(rings)) * pi;

        for (int seg = 0; seg < segments; ++seg)
        {
            float phi1 = (static_cast<float>(seg) / static_cast<float>(segments)) * 2.0f * pi;
            float phi2 = (static_cast<float>(seg + 1) / static_cast<float>(segments)) * 2.0f * pi;

            glm::vec3 p1 = center + radius * glm::vec3(sin(theta1) * cos(phi1), cos(theta1), sin(theta1) * sin(phi1));
            glm::vec3 p2 = center + radius * glm::vec3(sin(theta1) * cos(phi2), cos(theta1), sin(theta1) * sin(phi2));
            glm::vec3 p3 = center + radius * glm::vec3(sin(theta2) * cos(phi1), cos(theta2), sin(theta2) * sin(phi1));
            glm::vec3 p4 = center + radius * glm::vec3(sin(theta2) * cos(phi2), cos(theta2), sin(theta2) * sin(phi2));

            m_triangle_vertices.push_back({p1, color});
            m_triangle_vertices.push_back({p2, color});
            m_triangle_vertices.push_back({p3, color});

            m_triangle_vertices.push_back({p2, color});
            m_triangle_vertices.push_back({p4, color});
            m_triangle_vertices.push_back({p3, color});
        }
    }
}

void DebugRenderer::generate_cone_mesh(const glm::vec3& apex,
                                       const glm::vec3& axis,
                                       float height,
                                       float angle,
                                       const glm::vec4& color,
                                       int segments)
{
    const float pi = glm::pi<float>();

    glm::vec3 norm_axis = glm::normalize(axis);
    glm::vec3 up = (glm::abs(norm_axis.y) < 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(up, norm_axis));
    up = glm::cross(norm_axis, right);

    float base_radius = height * tan(angle);
    glm::vec3 base_center = apex + norm_axis * height;

    for (int seg = 0; seg < segments; ++seg)
    {
        float phi1 = (static_cast<float>(seg) / static_cast<float>(segments)) * 2.0f * pi;
        float phi2 = (static_cast<float>(seg + 1) / static_cast<float>(segments)) * 2.0f * pi;

        glm::vec3 p1 = base_center + base_radius * (cos(phi1) * right + sin(phi1) * up);
        glm::vec3 p2 = base_center + base_radius * (cos(phi2) * right + sin(phi2) * up);

        // Side triangle
        m_triangle_vertices.push_back({apex, color});
        m_triangle_vertices.push_back({p1, color});
        m_triangle_vertices.push_back({p2, color});

        // Base triangle
        m_triangle_vertices.push_back({base_center, color});
        m_triangle_vertices.push_back({p2, color});
        m_triangle_vertices.push_back({p1, color});
    }
}

void DebugRenderer::generate_frustum_lines(const glm::mat4& inv_view_proj, const glm::vec4& color)
{
    // NDC corners (using reversed-Z: near=1, far=0)
    glm::vec4 ndc_corners[8] = {
        {-1, -1, 1, 1},
        {1, -1, 1, 1},
        {1, 1, 1, 1},
        {-1, 1, 1, 1},  // Near
        {-1, -1, 0, 1},
        {1, -1, 0, 1},
        {1, 1, 0, 1},
        {-1, 1, 0, 1}  // Far
    };

    glm::vec3 world_corners[8];
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 world = inv_view_proj * ndc_corners[i];
        world_corners[i] = glm::vec3(world) / world.w;
    }

    // Near plane edges
    for (int i = 0; i < 4; ++i)
    {
        m_line_vertices.push_back({world_corners[i], color});
        m_line_vertices.push_back({world_corners[(i + 1) % 4], color});
    }

    // Far plane edges
    for (int i = 0; i < 4; ++i)
    {
        m_line_vertices.push_back({world_corners[4 + i], color});
        m_line_vertices.push_back({world_corners[4 + (i + 1) % 4], color});
    }

    // Connecting edges
    for (int i = 0; i < 4; ++i)
    {
        m_line_vertices.push_back({world_corners[i], color});
        m_line_vertices.push_back({world_corners[4 + i], color});
    }
}

void DebugRenderer::update_buffers()
{
    if (!m_buffers_dirty) return;

    Device& device = Engine::get().device();
    Context& ctx = device.get_context();

    // Update line buffer
    if (!m_line_vertices.empty())
    {
        size_t line_buffer_size = m_line_vertices.size() * sizeof(DebugVertex);

        if (m_line_vertex_buffer.buffer != VK_NULL_HANDLE)
        {
            ctx.deletion_queue.push_function([&device, buf = m_line_vertex_buffer] { device.destroy_buffer(buf); });
        }

        m_line_vertex_buffer = device.create_buffer(
            line_buffer_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(m_line_vertex_buffer.info.pMappedData, m_line_vertices.data(), line_buffer_size);

        VkBufferDeviceAddressInfo addr_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                            .buffer = m_line_vertex_buffer.buffer};
        m_line_vertex_buffer_address = vkGetBufferDeviceAddress(device.get(), &addr_info);
    }

    // Update triangle buffer
    if (!m_triangle_vertices.empty())
    {
        size_t triangle_buffer_size = m_triangle_vertices.size() * sizeof(DebugVertex);

        if (m_triangle_vertex_buffer.buffer != VK_NULL_HANDLE)
        {
            ctx.deletion_queue.push_function([&device, buf = m_triangle_vertex_buffer] { device.destroy_buffer(buf); });
        }

        m_triangle_vertex_buffer = device.create_buffer(
            triangle_buffer_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(m_triangle_vertex_buffer.info.pMappedData, m_triangle_vertices.data(), triangle_buffer_size);

        VkBufferDeviceAddressInfo addr_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                            .buffer = m_triangle_vertex_buffer.buffer};
        m_triangle_vertex_buffer_address = vkGetBufferDeviceAddress(device.get(), &addr_info);
    }

    m_buffers_dirty = false;
}

void DebugRenderer::render(const VkExtent2D& extent)
{
    if (m_line_vertices.empty() && m_triangle_vertices.empty()) return;

    update_buffers();

    Device& device = Engine::get().device();
    Scene& scene = Engine::get().scene();
    Context& ctx = device.get_context();

    struct DebugPushConstants
    {
        VkDeviceAddress vertices;
    };

    ctx.dcb.set_viewport(static_cast<float>(extent.width), static_cast<float>(extent.height));
    ctx.dcb.set_scissor(extent.width, extent.height);

    // Render solid triangles first (behind lines)
    if (!m_triangle_vertices.empty())
    {
        ctx.dcb.bind_pipeline(m_debug_solid_pipeline.get());

        VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_debug_solid_pipeline->get_set_layout(0));
        {
            DescriptorWriter writer;
            writer.write_buffer(0, scene.get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.update_set(device.get(), scene_descriptor);
        }
        ctx.dcb.bind_descriptors(scene_descriptor);

        DebugPushConstants push{};
        push.vertices = m_triangle_vertex_buffer_address;

        ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   sizeof(DebugPushConstants),
                                   &push);
        ctx.dcb.draw_auto(static_cast<uint32_t>(m_triangle_vertices.size()), 1, 0, 0);
    }

    // Render lines on top
    if (!m_line_vertices.empty())
    {
        ctx.dcb.bind_pipeline(m_debug_line_pipeline.get());

        VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_debug_line_pipeline->get_set_layout(0));
        {
            DescriptorWriter writer;
            writer.write_buffer(0, scene.get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.update_set(device.get(), scene_descriptor);
        }
        ctx.dcb.bind_descriptors(scene_descriptor);

        DebugPushConstants push{};
        push.vertices = m_line_vertex_buffer_address;

        ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   sizeof(DebugPushConstants),
                                   &push);
        ctx.dcb.draw_auto(static_cast<uint32_t>(m_line_vertices.size()), 1, 0, 0);
    }
}

void DebugRenderer::render_ui()
{
    if (ImGui::Begin("Debug Visualization"))
    {
        ImGui::SeparatorText("Visualization Options");

        ImGui::Checkbox("Show Camera Frustum", &m_settings.show_camera_frustum);
        if (m_settings.show_camera_frustum)
        {
            ImGui::Indent();
            ImGui::SliderFloat("Frustum Opacity", &m_settings.frustum_opacity, 0.1f, 1.0f);
            ImGui::Unindent();
        }

        ImGui::Checkbox("Show Meshlet Spheres", &m_settings.show_meshlet_spheres);
        if (m_settings.show_meshlet_spheres)
        {
            ImGui::Indent();
            ImGui::SliderFloat("Sphere Opacity", &m_settings.sphere_opacity, 0.1f, 1.0f);
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Green = Visible");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Red = Culled");
            ImGui::Unindent();
        }

        ImGui::Checkbox("Show Meshlet Cones", &m_settings.show_meshlet_cones);
        if (m_settings.show_meshlet_cones)
        {
            ImGui::Indent();
            ImGui::SliderFloat("Cone Opacity", &m_settings.cone_opacity, 0.1f, 1.0f);
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 0.8f, 1.0f), "Blue = Visible");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.1f, 1.0f), "Orange = Culled");
            ImGui::Unindent();
        }

        ImGui::Separator();

        if (ImGui::Checkbox("Pause Culling", &m_settings.pause_culling))
        {
            if (!m_settings.pause_culling)
            {
                m_has_cached_vp = false;  // Clear cache when unpausing
            }
        }
        if (m_settings.pause_culling)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(FROZEN)");
            ImGui::TextWrapped("Camera can move freely. Frustum shows frozen culling bounds.");
        }

        ImGui::SeparatorText("Statistics");

        // Meshlet stats
        if (m_settings.total_meshlet_count > 0)
        {
            float meshlet_ratio =
                static_cast<float>(m_settings.active_meshlet_count) / static_cast<float>(m_settings.total_meshlet_count);
            float cull_percent = (1.0f - meshlet_ratio) * 100.0f;

            ImGui::Text("Meshlets: %u / %u (%.1f%% culled)",
                        m_settings.active_meshlet_count,
                        m_settings.total_meshlet_count,
                        cull_percent);

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
            ImGui::ProgressBar(meshlet_ratio, ImVec2(-1, 0), "");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Text("Meshlets: 0 / 0");
        }

        // Instance stats
        if (m_settings.total_instance_count > 0)
        {
            float instance_ratio =
                static_cast<float>(m_settings.active_instance_count) / static_cast<float>(m_settings.total_instance_count);
            float cull_percent = (1.0f - instance_ratio) * 100.0f;

            ImGui::Text("Instances: %u / %u (%.1f%% culled)",
                        m_settings.active_instance_count,
                        m_settings.total_instance_count,
                        cull_percent);

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
            ImGui::ProgressBar(instance_ratio, ImVec2(-1, 0), "");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Text("Instances: 0 / 0");
        }
    }
    ImGui::End();
}