#pragma once


#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>
#include <string>

class Renderer2D;
class Camera;

/**
 * Tilemap
 *
 * Stores a flat grid of tile IDs.
 *   - Tile ID  0  = empty (no geometry, no collision)
 *   - Tile ID >0  = solid (blocks physics entities)
 *
 * ── Construction ─────────────────────────────────────────────────────────────
 *   // From a flat C-array / initialiser list (no file needed for quick tests):
 *   static const int tiles[] = { 0,0,0, 1,1,1 };   // row-major, top to bottom
 *   Tilemap level;
 *   level.load(25, 15, 48, tiles);
 *
 * ── Rendering ───────────────────────────────────────────────────────────────
 *   level.setTilesetTexture(tex.id, 8, 4);   // 8 cols × 4 rows in the sheet
 *   // Inside renderer.begin(cam) / renderer.end():
 *   level.draw(renderer);
 *   // Tiles without a tileset use a solid grey tint per tile ID.
 *
 * ── Collision ────────────────────────────────────────────────────────────────
 *   level.isSolid(tileX, tileY);             // grid-space lookup
 *   level.worldToTile(worldPos) → glm::ivec2
 *   level.tileToWorld(tileX, tileY) → glm::vec2   (top-left corner)
 */
class Tilemap
{
public:
    Tilemap() = default;

    // ── Loading ──────────────────────────────────────────────────────────────

    /**
     * Load from a flat row-major array of tile IDs.
     * @param cols      Number of columns (width in tiles)
     * @param rows      Number of rows    (height in tiles)
     * @param tileSize  Pixel size of one square tile
     * @param tileIDs   Flat array, length = cols*rows, row-major (top→bottom)
     */
    void load(int cols, int rows, int tileSize, const std::vector<int>& tileIDs);

    // ── Tileset ──────────────────────────────────────────────────────────────

    /**
     * Set the OpenGL texture used to render tiles.
     * tilesetCols/Rows describe how the sheet is divided.
     * Tile ID 1 maps to sheet index 0 (top-left), ID 2 → index 1, etc.
     * If not called, tiles are drawn as solid coloured quads.
     */
    void setTilesetTexture(GLuint textureID, int tilesetCols, int tilesetRows);

    // ── Queries ──────────────────────────────────────────────────────────────

    int  cols()     const { return m_cols; }
    int  rows()     const { return m_rows; }
    int  tileSize() const { return m_tileSize; }
    float worldWidth()  const { return static_cast<float>(m_cols * m_tileSize); }
    float worldHeight() const { return static_cast<float>(m_rows * m_tileSize); }

    /** Returns true if (tx, ty) is in bounds and has a non-zero tile ID. */
    bool isSolid(int tx, int ty) const;

    /** Clamp a world position into grid coordinates. */
    glm::ivec2 worldToTile(glm::vec2 worldPos) const;

    /** Top-left world corner of tile (tx, ty). */
    glm::vec2  tileToWorld(int tx, int ty)     const;

    int tileAt(int tx, int ty) const;

    // ── Rendering ────────────────────────────────────────────────────────────

    /** Draw all non-empty tiles. Must be called between renderer.begin/end. */
    void draw(Renderer2D& renderer) const;

private:
    int              m_cols     = 0;
    int              m_rows     = 0;
    int              m_tileSize = 32;
    std::vector<int> m_tiles;   // row-major: index = ty*m_cols + tx

    GLuint m_tilesetTexture  = 0;
    int    m_tilesetCols     = 1;
    int    m_tilesetRows     = 1;

    // Compute UV for a given tile ID in the sheet
    void tileUV(int tileID, glm::vec2& uvMin, glm::vec2& uvMax) const;
};
