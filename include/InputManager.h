#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include <array>
#include <unordered_map>
#include <string>
#include <functional>

/**
 * InputManager
 *
 * Unified polling-based input system. Call update() once per frame
 * BEFORE your game logic so all queries reflect the current frame.
 *
 * ── Keyboard ────────────────────────────────────────────────────────────────
 *   isKeyDown(SDL_SCANCODE_W)      true every frame the key is held
 *   isKeyPressed(SDL_SCANCODE_W)   true only on the frame the key went down
 *   isKeyReleased(SDL_SCANCODE_W)  true only on the frame the key came up
 *
 * ── Mouse ────────────────────────────────────────────────────────────────────
 *   mousePos()          current pixel position  (glm::ivec2)
 *   mouseDelta()        pixel movement this frame (glm::ivec2)
 *   isMouseDown(btn)    SDL_BUTTON_LEFT / _MIDDLE / _RIGHT
 *   isMousePressed(btn)
 *   isMouseReleased(btn)
 *   scrollDelta()       mouse wheel delta this frame
 *
 * ── Actions (named bindings) ─────────────────────────────────────────────────
 *   bindAction("jump", SDL_SCANCODE_SPACE);
 *   isActionPressed("jump");      ← same pressed/down/released semantics
 *
 * ── Gamepad (first connected controller) ─────────────────────────────────────
 *   isButtonDown(SDL_CONTROLLER_BUTTON_A)
 *   axisValue(SDL_CONTROLLER_AXIS_LEFTX)   → [-1.0, 1.0] with deadzone applied
 *
 * ── Misc ──────────────────────────────────────────────────────────────────────
 *   setMouseRelativeMode(true)    — lock + hide cursor (FPS camera)
 *   quit()                        — returns true if SDL_QUIT was received
 */
class InputManager
{
public:
    static constexpr int MAX_KEYS         = SDL_NUM_SCANCODES;
    static constexpr int MAX_MOUSE_BUTTONS = 6;   // SDL buttons are 1-indexed up to 5
    static constexpr float DEFAULT_DEADZONE = 0.15f;

    InputManager();
    ~InputManager();

    InputManager(const InputManager&)            = delete;
    InputManager& operator=(const InputManager&) = delete;

    // ------------------------------------------------------------------ //
    //  Frame lifecycle — call update() at the very start of every frame  //
    // ------------------------------------------------------------------ //

    /**
     * Feed a single SDL_Event into the InputManager.
     * Call inside your SDL_PollEvent loop.
     */
    void processEvent(const SDL_Event& e);

    /**
     * Advance the input state to the next frame.
     * Clears pressed/released flags and mouse delta.
     * Call BEFORE SDL_PollEvent.
     */
    void update();

    // ------------------------------------------------------------------ //
    //  Keyboard                                                           //
    // ------------------------------------------------------------------ //

    bool isKeyDown    (SDL_Scancode key) const;
    bool isKeyPressed (SDL_Scancode key) const;   // true for ONE frame
    bool isKeyReleased(SDL_Scancode key) const;   // true for ONE frame

    // ------------------------------------------------------------------ //
    //  Mouse                                                              //
    // ------------------------------------------------------------------ //

    glm::ivec2 mousePos()    const { return m_mousePos;    }
    glm::ivec2 mouseDelta()  const { return m_mouseDelta;  }
    glm::ivec2 scrollDelta() const { return m_scrollDelta; }

    bool isMouseDown    (int button) const;
    bool isMousePressed (int button) const;
    bool isMouseReleased(int button) const;

    /** Lock + hide cursor for relative (FPS) mouse mode. */
    void setMouseRelativeMode(bool enabled);
    bool isMouseRelative() const { return m_mouseRelative; }

    // ------------------------------------------------------------------ //
    //  Named action bindings                                              //
    // ------------------------------------------------------------------ //

    /** Map a human-readable action name to a scancode. */
    void bindAction(const std::string& action, SDL_Scancode key);

    bool isActionDown    (const std::string& action) const;
    bool isActionPressed (const std::string& action) const;
    bool isActionReleased(const std::string& action) const;

    // ------------------------------------------------------------------ //
    //  Gamepad                                                            //
    // ------------------------------------------------------------------ //

    bool  isControllerConnected() const { return m_controller != nullptr; }

    bool  isButtonDown    (SDL_GameControllerButton btn) const;
    bool  isButtonPressed (SDL_GameControllerButton btn) const;
    bool  isButtonReleased(SDL_GameControllerButton btn) const;

    /**
     * Returns axis value in [-1, 1] with deadzone applied.
     * Values inside the deadzone are returned as 0.
     */
    float axisValue(SDL_GameControllerAxis axis,
                    float deadzone = DEFAULT_DEADZONE) const;

    // ------------------------------------------------------------------ //
    //  Application                                                        //
    // ------------------------------------------------------------------ //

    bool quit() const { return m_quit; }

private:
    // Keyboard
    std::array<bool, MAX_KEYS> m_keysDown     {};
    std::array<bool, MAX_KEYS> m_keysPressed  {};
    std::array<bool, MAX_KEYS> m_keysReleased {};

    // Mouse
    std::array<bool, MAX_MOUSE_BUTTONS> m_mouseDown     {};
    std::array<bool, MAX_MOUSE_BUTTONS> m_mousePressed  {};
    std::array<bool, MAX_MOUSE_BUTTONS> m_mouseReleased {};

    glm::ivec2 m_mousePos    {0, 0};
    glm::ivec2 m_mouseDelta  {0, 0};
    glm::ivec2 m_scrollDelta {0, 0};
    bool       m_mouseRelative = false;

    // Named bindings
    std::unordered_map<std::string, SDL_Scancode> m_actionBindings;

    // Gamepad
    SDL_GameController* m_controller = nullptr;

    static constexpr int MAX_BUTTONS = SDL_CONTROLLER_BUTTON_MAX;
    static constexpr int MAX_AXES    = SDL_CONTROLLER_AXIS_MAX;

    std::array<bool, MAX_BUTTONS> m_btnDown     {};
    std::array<bool, MAX_BUTTONS> m_btnPressed  {};
    std::array<bool, MAX_BUTTONS> m_btnReleased {};

    // Misc
    bool m_quit = false;

    void openFirstController();
    void closeController();
};
