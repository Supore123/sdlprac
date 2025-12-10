#include "InputManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// -------------------------------------------------------------------------- //
//  Construction / destruction                                                 //
// -------------------------------------------------------------------------- //

InputManager::InputManager()
{
    m_keysDown.fill(false);
    m_keysPressed.fill(false);
    m_keysReleased.fill(false);
    m_mouseDown.fill(false);
    m_mousePressed.fill(false);
    m_mouseReleased.fill(false);
    m_btnDown.fill(false);
    m_btnPressed.fill(false);
    m_btnReleased.fill(false);

    openFirstController();
}

InputManager::~InputManager()
{
    closeController();
}

// -------------------------------------------------------------------------- //
//  Frame lifecycle                                                            //
// -------------------------------------------------------------------------- //

void InputManager::update()
{
    // Clear single-frame flags
    m_keysPressed.fill(false);
    m_keysReleased.fill(false);
    m_mousePressed.fill(false);
    m_mouseReleased.fill(false);
    m_btnPressed.fill(false);
    m_btnReleased.fill(false);
    m_mouseDelta  = {0, 0};
    m_scrollDelta = {0, 0};
}

void InputManager::processEvent(const SDL_Event& e)
{
    switch (e.type)
    {
        // ---------------------------------------------------------------- //
        //  Application                                                      //
        // ---------------------------------------------------------------- //
        case SDL_QUIT:
            m_quit = true;
            break;

        // ---------------------------------------------------------------- //
        //  Keyboard                                                         //
        // ---------------------------------------------------------------- //
        case SDL_KEYDOWN:
        {
            if (e.key.repeat != 0) break;   // ignore key-repeat events
            SDL_Scancode sc = e.key.keysym.scancode;
            if (sc >= 0 && sc < MAX_KEYS)
            {
                m_keysDown    [sc] = true;
                m_keysPressed [sc] = true;
            }
            break;
        }
        case SDL_KEYUP:
        {
            SDL_Scancode sc = e.key.keysym.scancode;
            if (sc >= 0 && sc < MAX_KEYS)
            {
                m_keysDown    [sc] = false;
                m_keysReleased[sc] = true;
            }
            break;
        }

        // ---------------------------------------------------------------- //
        //  Mouse                                                            //
        // ---------------------------------------------------------------- //
        case SDL_MOUSEMOTION:
            m_mousePos   = { e.motion.x,    e.motion.y    };
            m_mouseDelta = { e.motion.xrel,  e.motion.yrel };
            break;

        case SDL_MOUSEBUTTONDOWN:
        {
            int btn = e.button.button;
            if (btn > 0 && btn < MAX_MOUSE_BUTTONS)
            {
                m_mouseDown    [btn] = true;
                m_mousePressed [btn] = true;
            }
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            int btn = e.button.button;
            if (btn > 0 && btn < MAX_MOUSE_BUTTONS)
            {
                m_mouseDown    [btn] = false;
                m_mouseReleased[btn] = true;
            }
            break;
        }
        case SDL_MOUSEWHEEL:
            m_scrollDelta = { e.wheel.x, e.wheel.y };
            break;

        // ---------------------------------------------------------------- //
        //  Controller connection events                                     //
        // ---------------------------------------------------------------- //
        case SDL_CONTROLLERDEVICEADDED:
            if (!m_controller)
            {
                openFirstController();
            }
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            if (m_controller &&
                SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_controller))
                    == e.cdevice.which)
            {
                std::cout << "[InputManager] Controller disconnected.\n";
                closeController();
                openFirstController();
            }
            break;

        // ---------------------------------------------------------------- //
        //  Controller buttons                                               //
        // ---------------------------------------------------------------- //
        case SDL_CONTROLLERBUTTONDOWN:
        {
            int btn = e.cbutton.button;
            if (btn >= 0 && btn < MAX_BUTTONS)
            {
                m_btnDown    [btn] = true;
                m_btnPressed [btn] = true;
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP:
        {
            int btn = e.cbutton.button;
            if (btn >= 0 && btn < MAX_BUTTONS)
            {
                m_btnDown    [btn] = false;
                m_btnReleased[btn] = true;
            }
            break;
        }

        default:
            break;
    }
}

// -------------------------------------------------------------------------- //
//  Keyboard                                                                   //
// -------------------------------------------------------------------------- //

bool InputManager::isKeyDown    (SDL_Scancode key) const
{
    return (key >= 0 && key < MAX_KEYS) && m_keysDown[key];
}
bool InputManager::isKeyPressed (SDL_Scancode key) const
{
    return (key >= 0 && key < MAX_KEYS) && m_keysPressed[key];
}
bool InputManager::isKeyReleased(SDL_Scancode key) const
{
    return (key >= 0 && key < MAX_KEYS) && m_keysReleased[key];
}

// -------------------------------------------------------------------------- //
//  Mouse                                                                      //
// -------------------------------------------------------------------------- //

bool InputManager::isMouseDown    (int button) const
{
    return (button > 0 && button < MAX_MOUSE_BUTTONS) && m_mouseDown[button];
}
bool InputManager::isMousePressed (int button) const
{
    return (button > 0 && button < MAX_MOUSE_BUTTONS) && m_mousePressed[button];
}
bool InputManager::isMouseReleased(int button) const
{
    return (button > 0 && button < MAX_MOUSE_BUTTONS) && m_mouseReleased[button];
}

void InputManager::setMouseRelativeMode(bool enabled)
{
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
    m_mouseRelative = enabled;
}

// -------------------------------------------------------------------------- //
//  Named action bindings                                                      //
// -------------------------------------------------------------------------- //

void InputManager::bindAction(const std::string& action, SDL_Scancode key)
{
    m_actionBindings[action] = key;
}

bool InputManager::isActionDown(const std::string& action) const
{
    auto it = m_actionBindings.find(action);
    return it != m_actionBindings.end() && isKeyDown(it->second);
}
bool InputManager::isActionPressed(const std::string& action) const
{
    auto it = m_actionBindings.find(action);
    return it != m_actionBindings.end() && isKeyPressed(it->second);
}
bool InputManager::isActionReleased(const std::string& action) const
{
    auto it = m_actionBindings.find(action);
    return it != m_actionBindings.end() && isKeyReleased(it->second);
}

// -------------------------------------------------------------------------- //
//  Gamepad                                                                    //
// -------------------------------------------------------------------------- //

bool InputManager::isButtonDown    (SDL_GameControllerButton btn) const
{
    return (btn >= 0 && btn < MAX_BUTTONS) && m_btnDown[btn];
}
bool InputManager::isButtonPressed (SDL_GameControllerButton btn) const
{
    return (btn >= 0 && btn < MAX_BUTTONS) && m_btnPressed[btn];
}
bool InputManager::isButtonReleased(SDL_GameControllerButton btn) const
{
    return (btn >= 0 && btn < MAX_BUTTONS) && m_btnReleased[btn];
}

float InputManager::axisValue(SDL_GameControllerAxis axis, float deadzone) const
{
    if (!m_controller) return 0.0f;

    Sint16 raw   = SDL_GameControllerGetAxis(m_controller, axis);
    float  value = static_cast<float>(raw) / 32767.0f;
    value = std::clamp(value, -1.0f, 1.0f);

    // Apply deadzone: remap [deadzone, 1] → [0, 1] for smooth feel
    if (std::abs(value) < deadzone)
        return 0.0f;

    float sign  = (value > 0.0f) ? 1.0f : -1.0f;
    float scaled = (std::abs(value) - deadzone) / (1.0f - deadzone);
    return sign * scaled;
}

// -------------------------------------------------------------------------- //
//  Private helpers                                                            //
// -------------------------------------------------------------------------- //

void InputManager::openFirstController()
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            m_controller = SDL_GameControllerOpen(i);
            if (m_controller)
            {
                std::cout << "[InputManager] Controller connected: "
                          << SDL_GameControllerName(m_controller) << "\n";
                return;
            }
        }
    }
}

void InputManager::closeController()
{
    if (m_controller)
    {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }
}
