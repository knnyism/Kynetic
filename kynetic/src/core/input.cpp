//
// Created by kenny on 11/24/25.
//

#include "engine.hpp"
#include "device.hpp"
#include "input.hpp"

using namespace kynetic;

void Input::handle_event(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_EVENT_KEY_DOWN:
            m_states[event.key.scancode] |= Start | Down;
            break;
        case SDL_EVENT_KEY_UP:
            m_states[event.key.scancode] |= End;
            break;
        // case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (m_mouse_behavior == MouseBehavior::Locked)
            {
                m_mouse_delta.x += event.motion.xrel;
                m_mouse_delta.y += event.motion.yrel;
            }
        }
        break;

        default:
            break;
    }
}

void Input::update()
{
    m_mouse_delta = glm::vec2(0.f, 0.f);

    for (auto& state : m_states)
    {
        state &= ~Start;
        if (state & End) state &= ~Down & ~End;
    }
}

void Input::set_mouse_behavior(const MouseBehavior mouse_behavior)
{
    m_mouse_behavior = mouse_behavior;

    SDL_Window& window = Engine::get().device().get_window();

    switch (m_mouse_behavior)
    {
        case MouseBehavior::Free:
        {
            SDL_SetWindowRelativeMouseMode(&window, false);
        }
        break;
        case MouseBehavior::Locked:
        {
            SDL_SetWindowRelativeMouseMode(&window, true);
        }
        break;
    }
}
