#include "HUD.h"
#include "Renderer2D.h"
#include "Camera.h"

#include <algorithm>

// -------------------------------------------------------------------------- //
//  Frame API                                                                  //
// -------------------------------------------------------------------------- //

void HUD::begin(Renderer2D& renderer, const Camera& hudCamera)
{
    m_renderer = &renderer;
    renderer.begin(hudCamera);
}

void HUD::end()
{
    if (m_renderer)
        m_renderer->end();
    m_renderer = nullptr;
}

// -------------------------------------------------------------------------- //
//  Elements                                                                   //
// -------------------------------------------------------------------------- //

void HUD::drawHealthBar(glm::vec2 topLeft,
                        int current, int maximum,
                        float pipSize, float padding)
{
    if (!m_renderer || maximum <= 0) return;

    // Colours
    static const glm::vec4 k_full  = { 0.90f, 0.20f, 0.20f, 1.f };  // red heart
    static const glm::vec4 k_empty = { 0.35f, 0.15f, 0.15f, 0.8f }; // dark empty
    static const glm::vec4 k_border= { 0.10f, 0.05f, 0.05f, 1.f };  // thin outline

    float step = pipSize + padding;

    for (int i = 0; i < maximum; ++i)
    {
        glm::vec2 pos = { topLeft.x + i * step, topLeft.y };

        // Thin dark outline
        float b = 2.f;
        m_renderer->drawQuad(
            { pos.x - b, pos.y - b },
            { pipSize + b * 2.f, pipSize + b * 2.f },
            k_border);

        // Pip fill
        bool filled = (i < current);
        m_renderer->drawQuad(pos, { pipSize, pipSize },
                             filled ? k_full : k_empty);
    }
}

void HUD::drawPanel(glm::vec2 position, glm::vec2 size, glm::vec4 colour)
{
    if (!m_renderer) return;
    m_renderer->drawQuad(position, size, colour);
}


void HUD::drawProgressBar(glm::vec2 topLeft, glm::vec2 size, float fraction,
                           glm::vec4 fillColour, glm::vec4 emptyColour)
{
    if (!m_renderer) return;

    fraction = std::clamp(fraction, 0.f, 1.f);

    // Background (empty portion)
    m_renderer->drawQuad(topLeft, size, emptyColour);

    // Filled portion
    if (fraction > 0.f)
        m_renderer->drawQuad(topLeft, { size.x * fraction, size.y }, fillColour);
}
// Bitmap font layout assumed: 16 columns × 8 rows = 128 ASCII glyphs.
// The font sheet maps ASCII value → glyph index (0-based).
// Each glyph occupies (1/16) of sheet width and (1/8) of sheet height.
//
// Usage:  hud.drawText({20.f, 60.f}, "HP: 3/5", fontTexID, 16.f);
// Pass fontTexID = 0 to skip (graceful no-op when no font loaded).

void HUD::drawText(glm::vec2          position,
                   const std::string& text,
                   GLuint             fontTexID,
                   float              glyphW,
                   float              glyphH,
                   glm::vec4          colour)
{
    if (!m_renderer || fontTexID == 0 || text.empty()) return;

    constexpr int kSheetCols = 16;
    constexpr int kSheetRows = 8;

    float penX = position.x;

    for (char c : text)
    {
        // Map character to glyph index; clamp to printable ASCII range
        int idx = static_cast<unsigned char>(c);
        if (idx < 32 || idx > 127) idx = 32;   // fallback to space
        idx -= 32;                               // sheet starts at ASCII 32

        int col = idx % kSheetCols;
        int row = idx / kSheetCols;

        glm::vec2 uvMin = {
            static_cast<float>(col)     / kSheetCols,
            static_cast<float>(row)     / kSheetRows
        };
        glm::vec2 uvMax = {
            static_cast<float>(col + 1) / kSheetCols,
            static_cast<float>(row + 1) / kSheetRows
        };

        m_renderer->drawQuad({ penX, position.y }, { glyphW, glyphH },
                             fontTexID, colour, 0.f, uvMin, uvMax);
        penX += glyphW;
    }
}
