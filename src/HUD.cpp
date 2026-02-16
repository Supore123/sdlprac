
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
