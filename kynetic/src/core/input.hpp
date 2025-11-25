//
// Created by kenny on 11/24/25.
//

#pragma once
#include "SDL3/SDL_events.h"

namespace kynetic
{

class Input
{
    friend class Engine;
    friend class Device;

public:
    enum class MouseBehavior
    {
        Free,
        Locked
    };

private:
    enum KeyState
    {
        Down = 1 << 0,
        Start = 1 << 1,
        End = 1 << 2
    };

    int m_states[SDL_SCANCODE_COUNT]{};

    glm::vec2 m_mouse_delta{0.f};
    MouseBehavior m_mouse_behavior{MouseBehavior::Free};

    void handle_event(const SDL_Event& event);
    void update();

public:
    bool is_key_down(const SDL_Scancode scancode) const { return m_states[scancode] & Down; }
    bool is_key_start(const SDL_Scancode scancode) const { return m_states[scancode] & Start; }
    bool is_key_end(const SDL_Scancode scancode) const { return m_states[scancode] & End; }

    [[nodiscard]] glm::vec2 get_mouse_delta() const { return m_mouse_delta; }

    void set_mouse_behavior(MouseBehavior mouse_behavior);
};

}  // namespace kynetic
