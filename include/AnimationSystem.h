
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
 *   AnimationSystem::play(anim, "run");   // will restart if the clip isn't
 *                                       // already mid-play, otherwise does nothing
 */




#pragma once

#include <string>
class World;
class AnimationSystem
{
public:

