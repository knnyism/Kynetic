//
// Created by kenny on 11/4/25.
//
#include "core/components.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/renderer.hpp"
#include "core/resource_manager.hpp"

#include "rendering/model.hpp"

#include <random>
#include <vector>

class Viewer
{
    std::vector<flecs::entity> m_models;
    std::shared_ptr<kynetic::Model> m_bunny_model;
    flecs::entity m_camera;
    glm::vec3 m_camera_position{0.f, 2.f, 10.f};
    glm::vec3 m_camera_velocity{0.f};
    glm::quat m_camera_rotation{glm::identity<glm::quat>()};
    float m_yaw{0.f}, m_pitch{0.f};
    bool m_is_mouse_locked{true};

    // Source-style movement parameters
    float m_max_velocity{10.f};
    float m_accelerate{100.f};
    float m_friction{10.f};

    int m_grid_size{5};
    float m_spacing{2.0f};
    bool m_needs_rebuild{false};

    glm::vec3 accelerate(const glm::vec3& wish_dir, const glm::vec3& prev_velocity, float accel, float max_vel, float dt)
    {
        float proj_vel = glm::dot(prev_velocity, wish_dir);  // Project current velocity onto wish direction
        float accel_vel = accel * dt;                        // Acceleration this frame

        if (proj_vel + accel_vel > max_vel) accel_vel = max_vel - proj_vel;

        if (accel_vel < 0.f) accel_vel = 0.f;

        return prev_velocity + wish_dir * accel_vel;
    }

    glm::vec3 apply_friction(const glm::vec3& prev_velocity, float friction, float dt)
    {
        float speed = glm::length(prev_velocity);
        if (speed < 0.001f) return glm::vec3(0.f);

        float drop = speed * friction * dt;
        float new_speed = glm::max(speed - drop, 0.f);

        return prev_velocity * (new_speed / speed);
    }

public:
    Viewer()
    {
        kynetic::Scene& scene = kynetic::Engine::get().scene();
        kynetic::ResourceManager& resources = kynetic::Engine::get().resources();

        m_camera = scene.add_camera(true);
        m_bunny_model = resources.load<kynetic::Model>("assets/models/cube.glb");

        rebuild_grid();

        resources.refresh_mesh_buffers();
        resources.refresh_bindless_textures();
        resources.refresh_material_buffer();
    }

    void rebuild_grid()
    {
        kynetic::Scene& scene = kynetic::Engine::get().scene();

        for (auto& entity : m_models) entity.destruct();
        m_models.clear();

        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<float> rot_dist(0.f, glm::two_pi<float>());

        const float offset = (m_grid_size - 1) * m_spacing * 0.5f;

        for (int z = 0; z < m_grid_size; z++)
        {
            for (int x = 0; x < m_grid_size; x++)
            {
                flecs::entity entity = scene.add_model(m_bunny_model);

                glm::vec3 position{x * m_spacing - offset, 0.f, z * m_spacing - offset};

                glm::quat rotation = glm::quat(glm::vec3(rot_dist(rng), rot_dist(rng), rot_dist(rng)));

                entity.set<kynetic::TransformComponent>({.translation = position, .rotation = rotation});

                m_models.push_back(entity);
            }
        }
    }

    void update(float delta_time)
    {
        kynetic::Input& input = kynetic::Engine::get().input();

        // Mouse look
        const glm::vec2 delta = -glm::radians(input.get_mouse_delta() * 0.1f);
        m_yaw = fmodf(m_yaw + delta.x, glm::pi<float>() * 2.f);
        m_pitch = glm::clamp(m_pitch + delta.y, -glm::pi<float>() / 2.f, glm::pi<float>() / 2.f);

        const glm::quat rotation{glm::vec3(m_pitch, m_yaw, 0.f)};
        const glm::vec3 right = glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::vec3 up = glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 forward = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));

        // Build wish direction from input
        glm::vec3 wish_dir{0.f};

        if (input.is_key_down(SDL_SCANCODE_W)) wish_dir += forward;
        if (input.is_key_down(SDL_SCANCODE_S)) wish_dir -= forward;
        if (input.is_key_down(SDL_SCANCODE_D)) wish_dir += right;
        if (input.is_key_down(SDL_SCANCODE_A)) wish_dir -= right;
        if (input.is_key_down(SDL_SCANCODE_E)) wish_dir += up;
        if (input.is_key_down(SDL_SCANCODE_Q)) wish_dir -= up;

        float wish_speed = glm::length(wish_dir);
        if (wish_speed > 0.001f)
            wish_dir = glm::normalize(wish_dir);
        else
            wish_dir = glm::vec3(0.f);

        float max_vel = m_max_velocity;
        float accel = m_accelerate;
        if (input.is_key_down(SDL_SCANCODE_LSHIFT))
        {
            max_vel *= 4.f;
            accel *= 4.f;
        }

        m_camera_velocity = apply_friction(m_camera_velocity, m_friction, delta_time);

        if (wish_speed > 0.001f) m_camera_velocity = accelerate(wish_dir, m_camera_velocity, accel, max_vel, delta_time);

        m_camera_position += m_camera_velocity * delta_time;

        m_camera_rotation = rotation;
        m_camera.set<kynetic::TransformComponent>({.translation = m_camera_position, .rotation = m_camera_rotation});

        if (input.is_key_start(SDL_SCANCODE_F2)) m_is_mouse_locked = !m_is_mouse_locked;
        input.set_mouse_behavior(m_is_mouse_locked ? kynetic::Input::MouseBehavior::Locked
                                                   : kynetic::Input::MouseBehavior::Free);

        if (m_needs_rebuild)
        {
            rebuild_grid();
            m_needs_rebuild = false;
        }
    }

    void render()
    {
        ImGui::DockSpaceOverViewport(0, 0, ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::Begin("Viewer");
        {
            kynetic::Engine::get().renderer().render_imgui();

            if (ImGui::SliderInt("Grid Size", &m_grid_size, 1, 20)) m_needs_rebuild = true;
            if (ImGui::SliderFloat("Spacing", &m_spacing, 0.5f, 5.0f)) m_needs_rebuild = true;
            if (ImGui::Button("Regenerate")) m_needs_rebuild = true;
        }
        ImGui::End();
    }
};

Viewer* viewer = nullptr;
void update(float delta_time) { viewer->update(delta_time); }
void render() { viewer->render(); }

int main()
{
    kynetic::Engine engine;
    viewer = new Viewer();
    engine.set_update_callback(update);
    engine.set_render_callback(render);
    engine.init();
    engine.update();
    engine.shutdown();
    delete viewer;
    return 0;
}