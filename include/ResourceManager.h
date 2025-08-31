#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <string>
#include <unordered_map>
#include <stdexcept>

// -------------------------------------------------------------------------- //
//  Texture2D — lightweight wrapper around an OpenGL texture handle           //
// -------------------------------------------------------------------------- //

struct Texture2D
{
    GLuint      id     = 0;
    int         width  = 0;
    int         height = 0;
    int         channels = 0;   // 3 = RGB, 4 = RGBA

    bool isValid() const { return id != 0; }
};

// -------------------------------------------------------------------------- //
//  SoundEffect — wrapper around a Mix_Chunk (short, one-shot sounds)         //
// -------------------------------------------------------------------------- //

struct SoundEffect
{
    Mix_Chunk*  chunk = nullptr;
    bool isValid() const { return chunk != nullptr; }
};

// -------------------------------------------------------------------------- //
//  Music — wrapper around a Mix_Music (long, streamed tracks)                //
// -------------------------------------------------------------------------- //

struct Music
{
    Mix_Music*  track = nullptr;
    bool isValid() const { return track != nullptr; }
};

/**
 * ResourceManager
 *
 * Central, cache-first asset registry. All loaded assets are owned here
 * and released in one clean shutdown call.
 *
 * Supported asset types
 * ─────────────────────
 *   Texture2D   loadTexture(name, path, pixelated?)
 *   SoundEffect loadSoundEffect(name, path)
 *   Music       loadMusic(name, path)
 *
 * Usage
 * ─────
 *   ResourceManager res;
 *   res.initAudio();                                      // once at startup
 *
 *   Texture2D tex = res.loadTexture("hero", "res/hero.png");
 *   SoundEffect sfx = res.loadSoundEffect("jump", "res/jump.wav");
 *   Music bgm = res.loadMusic("theme", "res/theme.ogg");
 *
 *   // Later in render:
 *   glBindTexture(GL_TEXTURE_2D, res.getTexture("hero").id);
 *
 * All resources are freed by calling releaseAll() or when the
 * ResourceManager is destroyed.
 *
 * Dependencies: stb_image (header-only, place stb_image.h in include/)
 *               SDL2_mixer
 */
class ResourceManager
{
public:
    ResourceManager()  = default;
    ~ResourceManager() { releaseAll(); }

    ResourceManager(const ResourceManager&)            = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&)                 = default;
    ResourceManager& operator=(ResourceManager&&)      = default;

    // ------------------------------------------------------------------ //
    //  One-time audio subsystem init                                      //
    // ------------------------------------------------------------------ //

    /**
     * Initialise SDL2_mixer. Must be called once before any audio loads.
     * @param frequency  Sample rate in Hz       (default 44100)
     * @param channels   1 = mono, 2 = stereo    (default 2)
     * @param chunkSize  Audio buffer size        (default 2048)
     */
    bool initAudio(int frequency = 44100,
                   int channels  = 2,
                   int chunkSize = 2048);

    // ------------------------------------------------------------------ //
    //  Load / get — textures                                              //
    // ------------------------------------------------------------------ //

    /**
     * Load a texture from disk using stb_image. Returns a cached copy if
     * the name was already loaded.
     * @param pixelated  true → GL_NEAREST filtering (pixel art)
     *                   false → GL_LINEAR filtering (smooth)
     */
    Texture2D   loadTexture(const std::string& name,
                            const std::string& path,
                            bool pixelated = false);

    Texture2D   getTexture(const std::string& name) const;

    // ------------------------------------------------------------------ //
    //  Load / get — audio                                                 //
    // ------------------------------------------------------------------ //

    SoundEffect loadSoundEffect(const std::string& name,
                                const std::string& path);

    SoundEffect getSoundEffect(const std::string& name) const;

    Music       loadMusic(const std::string& name,
                          const std::string& path);

    Music       getMusic(const std::string& name) const;

    // ------------------------------------------------------------------ //
    //  Playback helpers (thin convenience wrappers around SDL_mixer)      //
    // ------------------------------------------------------------------ //

    /** Play a cached sound effect. channel = -1 picks first free channel. */
    void playSound(const std::string& name, int channel = -1, int loops = 0) const;

    /** Stream a cached music track. loops = -1 for infinite. */
    void playMusic(const std::string& name, int loops = -1) const;
    void pauseMusic()  const;
    void resumeMusic() const;
    void stopMusic()   const;

    // ------------------------------------------------------------------ //
    //  Lifetime                                                           //
    // ------------------------------------------------------------------ //

    /** Free every asset and clear the caches. */
    void releaseAll();

    /** Free only a specific texture by name. */
    void releaseTexture(const std::string& name);

    /** Free only a specific sound effect by name. */
    void releaseSoundEffect(const std::string& name);

    /** Free only a specific music track by name. */
    void releaseMusic(const std::string& name);

private:
    std::unordered_map<std::string, Texture2D>   m_textures;
    std::unordered_map<std::string, SoundEffect> m_sounds;
    std::unordered_map<std::string, Music>       m_music;

    bool m_audioReady = false;

    static Texture2D  loadTextureFromDisk(const std::string& path, bool pixelated);
};
