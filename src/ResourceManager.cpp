
// stb_image: single-header image loader.
// The IMPLEMENTATION define must appear in exactly one .cpp file.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "ResourceManager.h"
#include <iostream>

// -------------------------------------------------------------------------- //
//  Audio init                                                                 //
// -------------------------------------------------------------------------- //

bool ResourceManager::initAudio(int frequency, int channels, int chunkSize)
{
    if (m_audioReady)
        return true;

    if (Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, channels, chunkSize) < 0)
    {
        std::cerr << "[ResourceManager] Mix_OpenAudio failed: "
                  << Mix_GetError() << "\n";
        return false;
    }

    // Allocate mixing channels (16 simultaneous sound effects)
    Mix_AllocateChannels(16);
    m_audioReady = true;
    std::cout << "[ResourceManager] Audio subsystem ready.\n";
    return true;
}

// -------------------------------------------------------------------------- //
//  Textures                                                                   //
// -------------------------------------------------------------------------- //

Texture2D ResourceManager::loadTexture(const std::string& name,
                                       const std::string& path,
                                       bool pixelated)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        std::cout << "[ResourceManager] Texture '" << name
                  << "' already cached (ID " << it->second.id << ").\n";
        return it->second;
    }

    Texture2D tex = loadTextureFromDisk(path, pixelated);
    if (!tex.isValid())
    {
        std::cerr << "[ResourceManager] ERROR: Failed to load texture '"
                  << name << "' from '" << path << "'.\n";
        return tex;
    }

    m_textures[name] = tex;
    std::cout << "[ResourceManager] Texture '" << name
              << "' loaded (ID " << tex.id
              << ", " << tex.width << "x" << tex.height << ").\n";
    return tex;
}

Texture2D ResourceManager::getTexture(const std::string& name) const
{
    auto it = m_textures.find(name);
    if (it == m_textures.end())
    {
        std::cerr << "[ResourceManager] WARNING: Texture '" << name
                  << "' not found.\n";
        return Texture2D{};
    }
    return it->second;
}

// -------------------------------------------------------------------------- //
//  Sound effects                                                              //
// -------------------------------------------------------------------------- //

SoundEffect ResourceManager::loadSoundEffect(const std::string& name,
                                             const std::string& path)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
        return it->second;

    if (!m_audioReady)
    {
        std::cerr << "[ResourceManager] ERROR: Call initAudio() before loading sounds.\n";
        return SoundEffect{};
    }

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk)
    {
        std::cerr << "[ResourceManager] Mix_LoadWAV('" << path
                  << "') failed: " << Mix_GetError() << "\n";
        return SoundEffect{};
    }

    SoundEffect sfx{ chunk };
    m_sounds[name] = sfx;
    std::cout << "[ResourceManager] SoundEffect '" << name << "' loaded.\n";
    return sfx;
}

SoundEffect ResourceManager::getSoundEffect(const std::string& name) const
{
    auto it = m_sounds.find(name);
    if (it == m_sounds.end())
    {
        std::cerr << "[ResourceManager] WARNING: SoundEffect '" << name
                  << "' not found.\n";
        return SoundEffect{};
    }
    return it->second;
}

// -------------------------------------------------------------------------- //
//  Music                                                                      //
// -------------------------------------------------------------------------- //

Music ResourceManager::loadMusic(const std::string& name,
                                 const std::string& path)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
        return it->second;

    if (!m_audioReady)
    {
        std::cerr << "[ResourceManager] ERROR: Call initAudio() before loading music.\n";
        return Music{};
    }

    Mix_Music* track = Mix_LoadMUS(path.c_str());
    if (!track)
    {
        std::cerr << "[ResourceManager] Mix_LoadMUS('" << path
                  << "') failed: " << Mix_GetError() << "\n";
        return Music{};
    }

    Music m{ track };
    m_music[name] = m;
    std::cout << "[ResourceManager] Music '" << name << "' loaded.\n";
    return m;
}

Music ResourceManager::getMusic(const std::string& name) const
{
    auto it = m_music.find(name);
    if (it == m_music.end())
    {
        std::cerr << "[ResourceManager] WARNING: Music '" << name
                  << "' not found.\n";
        return Music{};
    }
    return it->second;
}

// -------------------------------------------------------------------------- //
//  Playback helpers                                                           //
// -------------------------------------------------------------------------- //

void ResourceManager::playSound(const std::string& name,
                                int channel, int loops) const
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end() && it->second.isValid())
        Mix_PlayChannel(channel, it->second.chunk, loops);
}

void ResourceManager::playMusic(const std::string& name, int loops) const
{
    auto it = m_music.find(name);
    if (it != m_music.end() && it->second.isValid())
        Mix_PlayMusic(it->second.track, loops);
}

void ResourceManager::pauseMusic()  const { Mix_PauseMusic();  }
void ResourceManager::resumeMusic() const { Mix_ResumeMusic(); }
void ResourceManager::stopMusic()   const { Mix_HaltMusic();   }

// -------------------------------------------------------------------------- //
//  Lifetime                                                                   //
// -------------------------------------------------------------------------- //

void ResourceManager::releaseTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end())
    {
        glDeleteTextures(1, &it->second.id);
        m_textures.erase(it);
    }
}

void ResourceManager::releaseSoundEffect(const std::string& name)
{
    auto it = m_sounds.find(name);
    if (it != m_sounds.end())
    {
        Mix_FreeChunk(it->second.chunk);
        m_sounds.erase(it);
    }
}

void ResourceManager::releaseMusic(const std::string& name)
{
    auto it = m_music.find(name);
    if (it != m_music.end())
    {
        Mix_FreeMusic(it->second.track);
        m_music.erase(it);
    }
}

void ResourceManager::releaseAll()
{
    for (auto& [name, tex] : m_textures)
        glDeleteTextures(1, &tex.id);
    m_textures.clear();

    for (auto& [name, sfx] : m_sounds)
        Mix_FreeChunk(sfx.chunk);
    m_sounds.clear();

    for (auto& [name, mus] : m_music)
        Mix_FreeMusic(mus.track);
    m_music.clear();

    if (m_audioReady)
    {
        Mix_CloseAudio();
        m_audioReady = false;
    }

    std::cout << "[ResourceManager] All assets released.\n";
}

// -------------------------------------------------------------------------- //
//  Private helpers                                                            //
// -------------------------------------------------------------------------- //

Texture2D ResourceManager::loadTextureFromDisk(const std::string& path,
                                               bool pixelated)
{
    Texture2D tex{};

    // stb_image loads top-left first; OpenGL expects bottom-left first
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path.c_str(),
                                    &tex.width, &tex.height,
                                    &tex.channels, 0);
    if (!data)
    {
        std::cerr << "[ResourceManager] stbi_load('" << path
                  << "') failed: " << stbi_failure_reason() << "\n";
        return tex;
    }

    GLenum internalFormat = (tex.channels == 4) ? GL_RGBA : GL_RGB;
    GLenum dataFormat     = internalFormat;

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexImage2D(GL_TEXTURE_2D, 0,
                 static_cast<GLint>(internalFormat),
                 tex.width, tex.height, 0,
                 dataFormat, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    GLint filter = pixelated ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    pixelated ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return tex;
}
