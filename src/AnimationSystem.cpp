
#include "AnimationSystem.h"
#include "ECS.h"
#include "Components.h"

#include <string>

// -------------------------------------------------------------------------- //
//  update                                                                     //
// -------------------------------------------------------------------------- //

void AnimationSystem::update(World& world, float dt) const
{
    // All entities that have both an Animator and a Sprite
    for (Entity e : world.view<AnimatorComponent, SpriteComponent>())
    {
        auto& anim   = world.get<AnimatorComponent>(e);
        auto& sprite = world.get<SpriteComponent>(e);

        if (!anim.playing) continue;

        auto clipIt = anim.clips.find(anim.currentClip);
        if (clipIt == anim.clips.end()) continue;

        const AnimationClip& clip = clipIt->second;
        if (clip.frames.empty()) continue;

        // Advance timer
        anim.timer += dt;

        float frameDuration = (clip.fps > 0.f) ? (1.f / clip.fps) : 0.f;

        if (frameDuration > 0.f && anim.timer >= frameDuration)
        {
            int frameCount = static_cast<int>(clip.frames.size());
            anim.timer -= frameDuration;
            anim.currentFrame++;

            if (anim.currentFrame >= frameCount)
            {
                anim.currentFrame = clip.loop ? 0 : frameCount - 1;
                anim.playing      = clip.loop;
            }
        }

        // Write UVs into SpriteComponent so Renderer2D picks them up
        const AnimationFrame& frame = clip.frames[
            static_cast<size_t>(anim.currentFrame)];
        sprite.uvMin = frame.uvMin;
        sprite.uvMax = frame.uvMax;
    }
}

// -------------------------------------------------------------------------- //
//  Static helper                                                              //
// -------------------------------------------------------------------------- //

void AnimationSystem::play(AnimatorComponent& anim, const std::string& clipName)
{
    if (anim.currentClip == clipName) return;       // already playing
    if (anim.clips.find(clipName) == anim.clips.end()) return;  // unknown clip

    anim.currentClip  = clipName;
    anim.currentFrame = 0;
    anim.timer        = 0.f;
    anim.playing      = true;
}
