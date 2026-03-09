



#pragma once
#include "ECS.h"
#include "Components.h"
#include "EnemySystem.h"
#include "Renderer2D.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <string>
#include <functional>
// ── Character type ────────────────────────────────────────────────────────────
enum class CharacterType { Knight, Ninja, Robot };
// ── Attack state attached to the player entity ───────────────────────────────
struct AttackComponent
{
    CharacterType character    = CharacterType::Knight;

    bool  attacking            = false;   // true during active attack window
    float attackTimer          = 0.f;    // counts up during attack
    float attackDuration       = 0.3f;   // seconds the attack lasts
    float attackCooldown       = 0.f;    // remaining cooldown
    float cooldownDuration     = 0.4f;   // seconds between attacks

    // Melee hitbox (Knight) — local offset + size relative to player pos
    glm::vec2 meleeOffset      = { 32.f, 8.f };
    glm::vec2 meleeSize        = { 36.f, 32.f };

    // Projectile (Ninja shuriken / Robot laser)
    bool      projectileFired  = false;

    // FX
    float     fxTimer          = 0.f;
    int       fxFrame          = 0;
};
