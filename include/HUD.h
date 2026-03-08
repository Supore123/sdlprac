#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

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
 *   Camera hudCam;
 *   hudCam.setMode(Camera::Mode::Orthographic);
 *   hudCam.setOrthoSize(1280.f, 720.f);
 *
 *   hud.begin(renderer, hudCam);
 *       hud.drawHealthBar({20, 20}, 3, 5);
 *       hud.drawPanel({10, 10}, {200, 40}, {0,0,0,0.6f});
 *       hud.drawText({20, 70}, "Score: 42", fontTex, 16.f);
 *   hud.end();
 *
 * begin/end wrap a Renderer2D batch — don't nest inside another batch.
 */
class HUD
{
public:
    HUD() = default;

    // ── Frame API ────────────────────────────────────────────────────────────

    void begin(Renderer2D& renderer, const Camera& hudCamera);
    void end();

    // ── Elements ─────────────────────────────────────────────────────────────

    void drawHealthBar(glm::vec2 topLeft,
                       int current, int maximum,
                       float pipSize = 24.f, float padding = 6.f);

    void drawPanel(glm::vec2 position, glm::vec2 size,
                   glm::vec4 colour = { 0.f, 0.f, 0.f, 0.55f });

    void drawProgressBar(glm::vec2 topLeft, glm::vec2 size, float fraction,
                         glm::vec4 fillColour  = { 0.2f, 0.8f, 0.3f, 1.f },
                         glm::vec4 emptyColour = { 0.2f, 0.2f, 0.2f, 0.8f });


private:
    Renderer2D* m_renderer = nullptr;
};
    /**
     * Draw a string using a bitmap font texture atlas (16 cols × 8 rows,
     * ASCII 32–127). Pass fontTexID = 0 to silently skip (no asset needed).
     * @param position   Top-left screen position of the first glyph
     * @param text       String to render
     * @param fontTexID  OpenGL texture ID of the font sheet (0 = no-op)
     * @param glyphW     Width  of one glyph in screen pixels (default 16)
     * @param glyphH     Height of one glyph in screen pixels (default 16)
     * @param colour     Tint colour (default white)
     */
    void drawText(glm::vec2          position,
                  const std::string& text,
                  GLuint             fontTexID,
                  float              glyphW   = 16.f,
                  float              glyphH   = 16.f,
                  glm::vec4          colour   = { 1.f, 1.f, 1.f, 1.f });
