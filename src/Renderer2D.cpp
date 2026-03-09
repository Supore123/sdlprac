#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <array>

// -------------------------------------------------------------------------- //
//  Built-in GLSL source (generated at runtime — no files needed)            //
// -------------------------------------------------------------------------- //

static const char* k_vertSrc = R"GLSL(
#version 410 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aTint;

out vec2 vUV;
out vec4 vTint;

uniform mat4 uViewProjection;

void main()
{
    vUV      = aUV;
    vTint    = aTint;
    gl_Position = uViewProjection * vec4(aPosition, 0.0, 1.0);
}
)GLSL";

static const char* k_fragSrc = R"GLSL(
#version 410 core

in  vec2 vUV;
in  vec4 vTint;
out vec4 FragColour;

uniform sampler2D uTexture;

void main()
{
    FragColour = texture(uTexture, vUV) * vTint;
}
)GLSL";

// -------------------------------------------------------------------------- //
//  Lifecycle                                                                  //
// -------------------------------------------------------------------------- //



// -------------------------------------------------------------------------- //
//  Frame API                                                                  //
// -------------------------------------------------------------------------- //


// -------------------------------------------------------------------------- //
//  Private helpers                                                            //
// -------------------------------------------------------------------------- //

bool Renderer2D::init(ShaderManager& shaderManager)
{
    if (m_initialised)
        return true;

    // Build the built-in shader from source strings
    m_shader = buildShader(shaderManager);
    if (m_shader == 0)
    {
        std::cerr << "[Renderer2D] Failed to build built-in shader.\n";
        return false;
    }

    // Pre-generate indices — pattern repeats for every quad:
    // 0,1,2  2,3,0
    std::vector<GLuint> indices;
    indices.reserve(MAX_INDICES);
    for (GLuint i = 0; i < static_cast<GLuint>(MAX_QUADS); ++i)
    {
        GLuint base = i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }

    // VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // VBO — dynamic, updated every frame
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(MAX_VERTICES * sizeof(Vertex)),
                 nullptr, GL_DYNAMIC_DRAW);

    // IBO — static, indices never change
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                 indices.data(), GL_STATIC_DRAW);

    // Vertex layout
    // location 0 — position  (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    // location 1 — uv        (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, uv)));
    // location 2 — tint      (vec4)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, tint)));

    glBindVertexArray(0);

    m_vertices.reserve(MAX_VERTICES);

    createWhiteTexture();

    // Bind texture unit 0 once — we only use one unit in 2D
    glUseProgram(m_shader);
    glUniform1i(glGetUniformLocation(m_shader, "uTexture"), 0);
    glUseProgram(0);

    m_initialised = true;
    std::cout << "[Renderer2D] Initialised (max " << MAX_QUADS << " quads/batch).\n";
    return true;
}

void Renderer2D::shutdown()
{
    if (!m_initialised) return;

    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
    glDeleteTextures(1, &m_whiteTexture);

    m_vao = m_vbo = m_ibo = m_whiteTexture = 0;
    m_initialised = false;
}

void Renderer2D::begin(const Camera& camera)
{
    glUseProgram(m_shader);
    glUniformMatrix4fv(
        glGetUniformLocation(m_shader, "uViewProjection"),
        1, GL_FALSE, glm::value_ptr(camera.getViewProjection()));

    m_vertices.clear();
    m_currentTexture = 0;
}

void Renderer2D::drawQuad(const glm::vec2& position,
                          const glm::vec2& size,
                          GLuint           textureID,
                          const glm::vec4& tint,
                          float            rotation,
                          const glm::vec2& uvMin,
                          const glm::vec2& uvMax)
{
    // Flush whenever texture changes
    if (m_currentTexture != 0 && textureID != m_currentTexture)
        flush();

    m_currentTexture = textureID;

    // Flush if batch is full
    if (m_vertices.size() + 4 > MAX_VERTICES)
        flush();

    // Build a local-space quad centred at origin, then transform
    glm::vec2 halfSize = size * 0.5f;
    glm::vec2 centre   = position + halfSize;

    // Local corners: TL, TR, BR, BL
    std::array<glm::vec2, 4> corners = {{
        { -halfSize.x, -halfSize.y },
        {  halfSize.x, -halfSize.y },
        {  halfSize.x,  halfSize.y },
        { -halfSize.x,  halfSize.y }
    }};

    // Apply rotation if needed
    if (rotation != 0.f)
    {
        float rad = glm::radians(rotation);
        float c   = std::cos(rad);
        float s   = std::sin(rad);

        for (auto& corner : corners)
        {
            float rx = corner.x * c - corner.y * s;
            float ry = corner.x * s + corner.y * c;
            corner = { rx, ry };
        }
    }

    // Translate to world position
    for (auto& corner : corners)
        corner += centre;

    // UV corners: TL, TR, BR, BL
    std::array<glm::vec2, 4> uvs = {{
        { uvMin.x, uvMin.y },
        { uvMax.x, uvMin.y },
        { uvMax.x, uvMax.y },
        { uvMin.x, uvMax.y }
    }};

    for (int i = 0; i < 4; ++i)
        m_vertices.push_back({ corners[i], uvs[i], tint });
}

void Renderer2D::drawQuad(const glm::vec2& position,
                          const glm::vec2& size,
                          const glm::vec4& colour,
                          float            rotation)
{
    drawQuad(position, size, m_whiteTexture, colour, rotation);
}

void Renderer2D::end()
{
    flush();
    glUseProgram(0);
}

