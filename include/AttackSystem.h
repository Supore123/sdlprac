



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
// ── Player projectile (separate from enemy projectile) ───────────────────────
struct PlayerProjectileComponent
{
    glm::vec2 velocity  = { 400.f, 0.f };
    float     lifetime  = 1.5f;
    float     damage    = 1.f;
};
// ── System ────────────────────────────────────────────────────────────────────
class AttackSystem
{
public:
    // Fired when a melee attack hits an enemy
    std::function<void(Entity player, Entity enemy)> onMeleeHit;
    // Fired when a player projectile hits an enemy
    std::function<void(Entity proj,   Entity enemy)> onProjectileHit;

    void update(World& world, Entity playerEntity, bool attackPressed, float dt);

    void render(World& world, Entity playerEntity,
                Renderer2D& renderer, const Camera& cam,
                GLuint fxSwordTex, GLuint fxHitTex,
                GLuint fxShurikenTex, GLuint fxLaserTex);

    // Call once in spawnPlayer after adding AttackComponent
    static void registerClips(World& world, Entity player, CharacterType type);

private:
    void fireProjectile(World& world, Entity player,
                        CharacterType type,
                        GLuint shurikenTex, GLuint laserTex);
    void checkMeleeHits(World& world, Entity player);
    void updateProjectiles(World& world, float dt);

    // Cached FX texture IDs set during render()
    GLuint m_fxSwordTex    = 0;
    GLuint m_fxShurikenTex = 0;
    GLuint m_fxLaserTex    = 0;
    GLuint m_fxHitTex      = 0;
};
