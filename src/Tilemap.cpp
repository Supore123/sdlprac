
#include "Tilemap.h"
#include "Renderer2D.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// -------------------------------------------------------------------------- //
//  Loading                                                                    //
// -------------------------------------------------------------------------- //

void Tilemap::load(int cols, int rows, int tileSize,
                   const std::vector<int>& tileIDs)
{
    m_cols     = cols;
    m_rows     = rows;
    m_tileSize = tileSize;
    m_tiles    = tileIDs;

    // Pad or trim to exact size
    m_tiles.resize(static_cast<size_t>(cols * rows), 0);

    std::cout << "[Tilemap] Loaded " << cols << "x" << rows
              << " grid, tileSize=" << tileSize << "px.\n";
}

// -------------------------------------------------------------------------- //
//  Tileset                                                                    //
// -------------------------------------------------------------------------- //

void Tilemap::setTilesetTexture(GLuint textureID, int tilesetCols, int tilesetRows)
{
    m_tilesetTexture = textureID;
    m_tilesetCols    = std::max(1, tilesetCols);
    m_tilesetRows    = std::max(1, tilesetRows);
}

// -------------------------------------------------------------------------- //
//  Queries                                                                    //
// -------------------------------------------------------------------------- //

bool Tilemap::isSolid(int tx, int ty) const
{
    if (tx < 0 || ty < 0 || tx >= m_cols || ty >= m_rows)
        return false;
    return m_tiles[static_cast<size_t>(ty * m_cols + tx)] != 0;
}

int Tilemap::tileAt(int tx, int ty) const
{
    if (tx < 0 || ty < 0 || tx >= m_cols || ty >= m_rows)
        return 0;
    return m_tiles[static_cast<size_t>(ty * m_cols + tx)];
}

glm::ivec2 Tilemap::worldToTile(glm::vec2 worldPos) const
{
    return {
        static_cast<int>(std::floor(worldPos.x / static_cast<float>(m_tileSize))),
        static_cast<int>(std::floor(worldPos.y / static_cast<float>(m_tileSize)))
    };
}

glm::vec2 Tilemap::tileToWorld(int tx, int ty) const
{
    return {
        static_cast<float>(tx * m_tileSize),
        static_cast<float>(ty * m_tileSize)
    };
}

// -------------------------------------------------------------------------- //
//  Rendering                                                                  //
// -------------------------------------------------------------------------- //

void Tilemap::tileUV(int tileID, glm::vec2& uvMin, glm::vec2& uvMax) const
{
    // tileID is 1-based; map to 0-based sheet index
    int idx  = tileID - 1;
    int col  = idx % m_tilesetCols;
    int row  = idx / m_tilesetCols;

    float colF = static_cast<float>(m_tilesetCols);
    float rowF = static_cast<float>(m_tilesetRows);

    uvMin = { static_cast<float>(col)     / colF,
              static_cast<float>(row)     / rowF };
    uvMax = { static_cast<float>(col + 1) / colF,
              static_cast<float>(row + 1) / rowF };
}

void Tilemap::draw(Renderer2D& renderer) const
{
    if (m_tiles.empty()) return;

    float ts = static_cast<float>(m_tileSize);

    for (int ty = 0; ty < m_rows; ++ty)
    {
        for (int tx = 0; tx < m_cols; ++tx)
        {
            int id = tileAt(tx, ty);
            if (id == 0) continue;

            glm::vec2 worldPos = tileToWorld(tx, ty);
            glm::vec2 tileVec  = { ts, ts };

            if (m_tilesetTexture != 0)
            {
                glm::vec2 uvMin, uvMax;
                tileUV(id, uvMin, uvMax);
                renderer.drawQuad(worldPos, tileVec, m_tilesetTexture,
                                  glm::vec4(1.f), 0.f, uvMin, uvMax);
            }
            else
            {
                // No tileset — draw tinted solid quads so the level is
                // immediately visible without any asset files.
                // Tile ID 1 = stone grey, 2 = brown dirt, 3 = dark rock, etc.
                static const glm::vec4 k_palette[] = {
                    { 0.55f, 0.55f, 0.60f, 1.f },   // 1  stone
                    { 0.55f, 0.38f, 0.22f, 1.f },   // 2  dirt
                    { 0.30f, 0.28f, 0.32f, 1.f },   // 3  dark rock
                    { 0.20f, 0.60f, 0.25f, 1.f },   // 4  grass top
                    { 0.70f, 0.60f, 0.20f, 1.f },   // 5  sand
                };
                int palIdx = std::clamp(id - 1, 0, 4);
                renderer.drawQuad(worldPos, tileVec, k_palette[palIdx]);
            }
        }
    }
}
