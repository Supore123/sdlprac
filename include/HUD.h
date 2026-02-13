#pragma once


#include <GL/glew.h>
#include <glm/glm.hpp>

class Renderer2D;
class Camera;

/**
 * HUD
 *
 * Renders heads-up display elements using plain Renderer2D quads so no
 * font or atlas assets are required. Add bitmap-font text rendering later
 * by extending drawText().
 *
 * ── Usage ────────────────────────────────────────────────────────────────────
 *   // Create a screen-space orthographic camera once:
 *   Camera hudCam;
 *   hudCam.setMode(Camera::Mode::Orthographic);
 *   hudCam.setOrthoSize(1280.f, 720.f);
 *
 *   // In render():
 *   hud.begin(renderer, hudCam);
 *       hud.drawHealthBar({20, 20}, 3, 5);       // 3/5 hearts
 *       hud.drawPanel({10, 10}, {200, 40}, {0,0,0,0.6f});
 *   hud.end();
 *
 * begin/end wrap a Renderer2D batch — don't nest inside another batch.
 */
class HUD
{
public:
    HUD() = default;

    // ── Frame API ────────────────────────────────────────────────────────────

    /** Begin a screen-space HUD batch. hudCamera must be orthographic. */
    void begin(Renderer2D& renderer, const Camera& hudCamera);

    /** Flush and finish the HUD batch. */
    void end();

    // ── Elements ─────────────────────────────────────────────────────────────

    /**
     * Draw a row of heart / pip health indicators.
     * @param topLeft     Top-left screen position
     * @param current     Filled pips
     * @param maximum     Total pips
     * @param pipSize     Width and height of each pip square (default 24)
     * @param padding     Gap between pips (default 6)
     */
    void drawHealthBar(glm::vec2 topLeft,
                       int current, int maximum,
                       float pipSize = 24.f, float padding = 6.f);

    /**
     * Draw a semi-transparent panel (background rect).
     * Useful for score boxes, dialogue frames, etc.
     */
    void drawPanel(glm::vec2 position, glm::vec2 size,
                   glm::vec4 colour = { 0.f, 0.f, 0.f, 0.55f });

    /**
     * Draw a horizontal progress bar (e.g. stamina, XP).
     * @param topLeft   Top-left screen corner
     * @param size      Total bar dimensions
     * @param fraction  Fill fraction [0, 1]
     * @param fillColour Colour of the filled portion
     */
    void drawProgressBar(glm::vec2 topLeft, glm::vec2 size, float fraction,
                         glm::vec4 fillColour  = { 0.2f, 0.8f, 0.3f, 1.f },
                         glm::vec4 emptyColour = { 0.2f, 0.2f, 0.2f, 0.8f });

private:
    Renderer2D* m_renderer = nullptr;
};
