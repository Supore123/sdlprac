#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Camera.h"
#include "ResourceManager.h"
#include "ShaderManager.h"

#include <vector>
#include <string>

/**
 * Renderer2D
 *
 * Batched 2D sprite renderer. Accumulates draw calls each frame into a
 * single VBO upload, minimising draw calls to one per texture atlas/swap.
 *
 * ── Lifecycle ──────────────────────────────────────────────────────────────
 *   1. Call init(shaderManager) once after the GL context is ready.
 *   2. Each frame:
 *        renderer.begin(camera);
 *          renderer.drawQuad({100,100}, {64,64}, tex.id);
 *          renderer.drawQuad({200,100}, {64,64}, tex.id, {1,0,0,1});  // red tint
 *        renderer.end();
 *
 * ── Coordinate system ───────────────────────────────────────────────────────
 *   Uses the camera's orthographic projection. Pass a Camera configured
 *   with setOrthoSize(screenW, screenH) for pixel-space coordinates.
 *
 * ── Batching ────────────────────────────────────────────────────────────────
 *   All quads submitted between begin() / end() with the same texture are
 *   flushed in one draw call. A texture change triggers an automatic flush.
 *   The batch is also flushed automatically on end().
 *
 * ── Built-in shader ─────────────────────────────────────────────────────────
 *   Renderer2D generates its own GLSL shader source at runtime — no .vert /
 *   .frag files needed. The shader is registered in ShaderManager as
 *   "renderer2d_builtin" so you can override it later if needed.
 */
class Renderer2D
{
public:
    static constexpr size_t MAX_QUADS    = 10000;
    static constexpr size_t MAX_VERTICES = MAX_QUADS * 4;
    static constexpr size_t MAX_INDICES  = MAX_QUADS * 6;

    Renderer2D()  = default;
    ~Renderer2D() { shutdown(); }

    Renderer2D(const Renderer2D&)            = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    // ------------------------------------------------------------------ //
    //  Lifecycle                                                           //
    // ------------------------------------------------------------------ //

    /**
     * Allocate GPU resources and compile the built-in shader.
     * Must be called once after glewInit().
     */
    bool init(ShaderManager& shaderManager);

    /** Release all GPU resources. */
    void shutdown();

    // ------------------------------------------------------------------ //
    //  Frame API                                                           //
    // ------------------------------------------------------------------ //

    /** Begin a new batch. Must be called before any drawQuad(). */
    void begin(const Camera& camera);

    /**
     * Queue a textured quad.
     * @param position  Top-left corner in screen pixels (or world units)
     * @param size      Width / height in pixels
     * @param textureID OpenGL texture handle (from Texture2D::id)
     * @param tint      RGBA colour multiplier (default white = no tint)
     * @param rotation  Clockwise rotation in degrees around the quad centre
     * @param uvMin     Bottom-left UV coordinate  (default {0,0})
     * @param uvMax     Top-right UV coordinate     (default {1,1})
     */
    void drawQuad(const glm::vec2& position,
                  const glm::vec2& size,
                  GLuint           textureID,
                  const glm::vec4& tint     = glm::vec4(1.f),
                  float            rotation = 0.f,
                  const glm::vec2& uvMin    = glm::vec2(0.f),
                  const glm::vec2& uvMax    = glm::vec2(1.f));

    /**
     * Queue a flat coloured quad (no texture).
     * Internally uses a 1×1 white texture so it goes through the same path.
     */
    void drawQuad(const glm::vec2& position,
                  const glm::vec2& size,
                  const glm::vec4& colour,
                  float            rotation = 0.f);

    /** Flush remaining quads and finish the batch. */
    void end();

    // ------------------------------------------------------------------ //
    //  Stats                                                               //
    // ------------------------------------------------------------------ //

    struct Stats
    {
        uint32_t drawCalls  = 0;
        uint32_t quadCount  = 0;
    };

    Stats getStats()    const { return m_stats; }
    void  resetStats()        { m_stats = {}; }

private:
    // Per-vertex layout uploaded to the GPU
    struct Vertex
    {
        glm::vec2 position;
        glm::vec2 uv;
        glm::vec4 tint;
    };

    // GPU objects
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ibo = 0;

    // 1×1 white fallback texture for colour-only quads
    GLuint m_whiteTexture = 0;

    // Shader
    GLuint m_shader = 0;

    // CPU-side batch buffer
    std::vector<Vertex>   m_vertices;
    GLuint                m_currentTexture = 0;

    Stats m_stats;

    bool m_initialised = false;

    void flush();
    void createWhiteTexture();
    static GLuint buildShader(ShaderManager& sm);
};
