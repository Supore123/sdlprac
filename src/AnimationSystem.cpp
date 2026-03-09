
// START_COMMIT_1
#include "AnimationSystem.h"
#include "ECS.h"
#include "Components.h"

#include <string>
// END_COMMIT_1

// -------------------------------------------------------------------------- //
//  update                                                                     //
// -------------------------------------------------------------------------- //

// START_COMMIT_2
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

        // Advance timer; support large dt by looping so we don't skip
        // several frames if the game lags or the frametime spikes.
        anim.timer += dt;

        float frameDuration = (clip.fps > 0.f) ? (1.f / clip.fps) : 0.f;
        if (frameDuration > 0.f)
        {
            int frameCount = static_cast<int>(clip.frames.size());
            // consume as many frame intervals as we have time for
            while (anim.timer >= frameDuration && anim.playing)
            {
                anim.timer -= frameDuration;
                anim.currentFrame++;

                if (anim.currentFrame >= frameCount)
                {
                    if (clip.loop)
                    {
                        anim.currentFrame = 0;
                    }
                    else
                    {
                        anim.currentFrame = frameCount - 1;
                        anim.playing      = false;
                        break;
                    }
                }
            }
        }
// END_COMMIT_2

// START_COMMIT_3
        // Write UVs into SpriteComponent so Renderer2D picks them up
        const AnimationFrame& frame = clip.frames[
            static_cast<size_t>(anim.currentFrame)];
        sprite.uvMin = frame.uvMin;
        sprite.uvMax = frame.uvMax;
    }
}
// END_COMMIT_3

// -------------------------------------------------------------------------- //
//  Static helper                                                              //
// -------------------------------------------------------------------------- //

// START_COMMIT_4
void AnimationSystem::play(AnimatorComponent& anim, const std::string& clipName)
{
    // if the clip doesn't exist we do nothing
    auto it = anim.clips.find(clipName);
    if (it == anim.clips.end())
        return;

    // if we're already playing the requested clip, leave it alone
    // (calling again on a non-playing clip will restart it)
    if (anim.currentClip == clipName && anim.playing)
        return;

    anim.currentClip  = clipName;
    anim.currentFrame = 0;
    anim.timer        = 0.f;
    anim.playing      = true;
}
// END_COMMIT_4
