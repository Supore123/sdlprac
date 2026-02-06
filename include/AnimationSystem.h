#pragma once

#include <string>
class World;

/**
 * AnimationSystem
 *
 * Advances AnimatorComponent frame timers and writes the current frame's
 * UV coordinates back into the entity's SpriteComponent each tick.
 *
 * ── Setup ────────────────────────────────────────────────────────────────────
 *   // Build clips
 *   AnimationClip idle;
 *   idle.name = "idle";
 *   idle.fps  = 6.f;
 *   idle.loop = true;
 *   idle.frames = { {{0.f,0.f},{0.25f,1.f}}, {{0.25f,0.f},{0.5f,1.f}} };
 *
 *   auto& anim = world.add<AnimatorComponent>(player);
 *   anim.clips["idle"] = idle;
 *   anim.currentClip   = "idle";
 *
 * ── Per-frame ────────────────────────────────────────────────────────────────
 *   animSystem.update(world, dt);
 *
 * ── Switching clips ──────────────────────────────────────────────────────────
 *   // From any system or state:
 *   AnimationSystem::play(anim, "run");   // restarts if same clip, no-op if already playing
 */
class AnimationSystem
{
public:
    /** Advance all AnimatorComponents and sync UVs to SpriteComponents. */
    void update(World& world, float dt) const;

    /**
     * Request a clip change on an AnimatorComponent.
     * If the clip is already playing it is not restarted.
     * No-op if the clip name doesn't exist in the clips map.
     */
    static void play(struct AnimatorComponent& anim, const std::string& clipName);
};
