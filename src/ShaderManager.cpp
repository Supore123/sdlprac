#include "ShaderManager.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

// -------------------------------------------------------------------------- //
//  Public interface                                                           //
// -------------------------------------------------------------------------- //

GLuint ShaderManager::loadProgram(const std::string& name,
                                  const std::string& vertPath,
                                  const std::string& fragPath)
{
    // Return cached program if already loaded
    auto it = m_programs.find(name);
    if (it != m_programs.end())
    {
        std::cout << "[ShaderManager] Program '" << name << "' already loaded (ID "
                  << it->second << ").\n";
        return it->second;
    }

    std::string vertSrc, fragSrc;
    try
    {
        vertSrc = readFile(vertPath);
        fragSrc = readFile(fragPath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ShaderManager] ERROR reading shader files: " << e.what() << "\n";
        return 0;
    }

    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    if (vert == 0 || frag == 0)
    {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return 0;
    }

    GLuint program = linkProgram(vert, frag);

    // Shader objects are no longer needed once linked
    glDeleteShader(vert);
    glDeleteShader(frag);

    if (program == 0)
        return 0;

    m_programs[name] = program;
    std::cout << "[ShaderManager] Program '" << name << "' loaded (ID " << program << ").\n";
    return program;
}

GLuint ShaderManager::getProgram(const std::string& name) const
{
    auto it = m_programs.find(name);
    if (it == m_programs.end())
    {
        std::cerr << "[ShaderManager] WARNING: Program '" << name << "' not found.\n";
        return 0;
    }
    return it->second;
}

void ShaderManager::useProgram(const std::string& name) const
{
    GLuint id = getProgram(name);
    if (id != 0)
        glUseProgram(id);
}

void ShaderManager::useProgram(GLuint programID)
{
    glUseProgram(programID);
}

void ShaderManager::deleteProgram(const std::string& name)
{
    auto it = m_programs.find(name);
    if (it != m_programs.end())
    {
        glDeleteProgram(it->second);
        m_programs.erase(it);
    }
}

void ShaderManager::deleteAll()
{
    for (auto& [name, id] : m_programs)
        glDeleteProgram(id);
    m_programs.clear();
}

// -------------------------------------------------------------------------- //
//  Uniform setters                                                            //
// -------------------------------------------------------------------------- //

void ShaderManager::setUniform(GLuint program, const std::string& name, bool value)
{
    glUniform1i(glGetUniformLocation(program, name.c_str()), static_cast<int>(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(program, name.c_str()), value);
}

void ShaderManager::setUniform(GLuint program, const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(program, name.c_str()), value);
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::vec2& value)
{
    glUniform2fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::vec3& value)
{
    glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::vec4& value)
{
    glUniform4fv(glGetUniformLocation(program, name.c_str()), 1, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::mat3& value)
{
    glUniformMatrix3fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderManager::setUniform(GLuint program, const std::string& name, const glm::mat4& value)
{
    glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

// -------------------------------------------------------------------------- //
//  Private helpers                                                            //
// -------------------------------------------------------------------------- //

std::string ShaderManager::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Could not open file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint ShaderManager::compileShader(GLenum type, const std::string& source)
{
    GLuint shader    = glCreateShader(type);
    const char* src  = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint ShaderManager::linkProgram(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    checkCompileErrors(program, "PROGRAM");

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void ShaderManager::checkCompileErrors(GLuint object, const std::string& type)
{
    GLint  success = 0;
    GLchar log[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, sizeof(log), nullptr, log);
            std::cerr << "[ShaderManager] COMPILE ERROR (" << type << "):\n"
                      << log << "\n";
        }
    }
    else
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, sizeof(log), nullptr, log);
            std::cerr << "[ShaderManager] LINK ERROR:\n" << log << "\n";
        }
    }
}
