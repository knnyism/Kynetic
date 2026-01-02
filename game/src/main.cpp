//
// Created by kenny on 11/4/25.
//

#include "core/components.hpp"
#include "core/engine.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/resource_manager.hpp"

#include "rendering/model.hpp"

class Viewer
{
    flecs::entity m_model;
    flecs::entity m_camera;

    glm::vec3 m_camera_position{0.f, 0.5f, 3.f};
    glm::quat m_camera_rotation{glm::identity<glm::quat>()};

    float m_yaw{0.f}, m_pitch{0.f};

    bool m_is_mouse_locked{true};

public:
    Viewer()
    {
        kynetic::Scene& scene = kynetic::Engine::get().scene();
        kynetic::ResourceManager& resources = kynetic::Engine::get().resources();

        m_camera = scene.add_camera(true);

#if 1
        m_model = scene.add_model(resources.load<kynetic::Model>("assets/models/DragonAttenuation.glb"));
#else
        m_model = scene.add_model(resources.load<kynetic::Model>("assets/models/bistro/Bistro.gltf"));
#endif
        resources.refresh_mesh_buffers();
        resources.refresh_bindless_textures();
        resources.refresh_material_buffer();
    }

    void update(float delta_time)
    {
        kynetic::Input& input = kynetic::Engine::get().input();

        const glm::vec2 delta = -glm::radians(input.get_mouse_delta() * 0.1f);
        m_yaw = fmodf(m_yaw + delta.x, glm::pi<float>() * 2.f);
        m_pitch = glm::clamp(m_pitch + delta.y, -glm::pi<float>() / 2.f, glm::pi<float>() / 2.f);

        const glm::quat rotation{glm::vec3(m_pitch, m_yaw, 0.f)};

        const glm::vec3 right = glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::vec3 up = glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::vec3 forward = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec3 input_direction{input.is_key_down(SDL_SCANCODE_D) - input.is_key_down(SDL_SCANCODE_A),
                                  input.is_key_down(SDL_SCANCODE_E) - input.is_key_down(SDL_SCANCODE_Q),
                                  input.is_key_down(SDL_SCANCODE_S) - input.is_key_down(SDL_SCANCODE_W)};
        input_direction *= 5.f;

        m_camera_position += right * input_direction.x * delta_time;
        m_camera_position += up * input_direction.y * delta_time;
        m_camera_position += forward * input_direction.z * delta_time;

        m_camera_rotation = glm::quat(glm::vec3(m_pitch, m_yaw, 0.f));

        m_camera.set<kynetic::TransformComponent>({.translation = m_camera_position, .rotation = m_camera_rotation});

        if (input.is_key_start(SDL_SCANCODE_F2)) m_is_mouse_locked = !m_is_mouse_locked;

        input.set_mouse_behavior(m_is_mouse_locked ? kynetic::Input::MouseBehavior::Locked
                                                   : kynetic::Input::MouseBehavior::Free);
    }

    void render() { ImGui::ShowDemoWindow(); }
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