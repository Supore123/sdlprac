#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

/**
 * ShaderManager
 *
 * Handles the full lifecycle of OpenGL shader programs:
 *   - Reading GLSL source from disk
 *   - Compiling vertex and fragment shaders
 *   - Linking into a program
 *   - Caching programs by a human-readable name
 *   - Providing typed uniform setters
 *
 * Usage:
 *   ShaderManager shaders;
 *   GLuint prog = shaders.loadProgram("basic", "res/basic.vert", "res/basic.frag");
 *   shaders.useProgram("basic");
 *   shaders.setUniform(prog, "uColor", glm::vec4(1.f, 0.f, 0.f, 1.f));
 */
class ShaderManager
{
public:
    ShaderManager()  = default;
    ~ShaderManager() { deleteAll(); }

    // Non-copyable, movable
    ShaderManager(const ShaderManager&)            = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&)                 = default;
    ShaderManager& operator=(ShaderManager&&)      = default;

    /**
     * Load, compile, and link a shader program from source files.
     * If a program with this name already exists it is returned directly.
     * Returns 0 on failure.
     */
    GLuint loadProgram(const std::string& name,
                       const std::string& vertPath,
                       const std::string& fragPath);

    /** Retrieve a cached program by name. Returns 0 if not found. */
    GLuint getProgram(const std::string& name) const;

    /** glUseProgram wrapper by name. No-op if name not found. */
    void useProgram(const std::string& name) const;

    /** glUseProgram wrapper by raw ID. */
    static void useProgram(GLuint programID);

    /** Remove one cached program and free its GPU resource. */
    void deleteProgram(const std::string& name);

    /** Remove all cached programs and free GPU resources. */
    void deleteAll();

    // ------------------------------------------------------------------ //
    //  Uniform setters — call after binding the program you want to write //
    // ------------------------------------------------------------------ //
    static void setUniform(GLuint program, const std::string& name, bool        value);
    static void setUniform(GLuint program, const std::string& name, int         value);
    static void setUniform(GLuint program, const std::string& name, float       value);
    static void setUniform(GLuint program, const std::string& name, const glm::vec2& value);
    static void setUniform(GLuint program, const std::string& name, const glm::vec3& value);
    static void setUniform(GLuint program, const std::string& name, const glm::vec4& value);
    static void setUniform(GLuint program, const std::string& name, const glm::mat3& value);
    static void setUniform(GLuint program, const std::string& name, const glm::mat4& value);

private:
    std::unordered_map<std::string, GLuint> m_programs;

    static std::string  readFile(const std::string& path);
    static GLuint       compileShader(GLenum type, const std::string& source);
    static GLuint       linkProgram(GLuint vert, GLuint frag);
    static void         checkCompileErrors(GLuint object, const std::string& type);
};
