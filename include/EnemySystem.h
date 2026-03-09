#pragma once
#include "ECS.h"
#include "Components.h"
#include "Tilemap.h"
#include <glm/glm.hpp>
#include <functional>

// ── Enemy type tag ────────────────────────────────────────────────────────────
enum class EnemyType { Patroller, Flyer, Shooter, Jumper };

struct EnemyComponent
{
    EnemyType type          = EnemyType::Patroller;
    float     health        = 3.f;
    float     maxHealth     = 3.f;
    float     speed         = 60.f;
    float     aggroRange    = 300.f;   // px — how close player must be to react
    float     timer         = 0.f;     // general-purpose per-enemy timer
    float     hitFlashTimer = 0.f;     // seconds — red flash when hit
    float     patrolLeft    = 0.f;     // patrol bounds (set at spawn)
    float     patrolRight   = 0.f;
    bool      facingRight   = true;
    bool      alive         = true;
};

struct ProjectileComponent
{
    glm::vec2 velocity    = { 300.f, 0.f };
    float     lifetime    = 2.5f;     // seconds before despawn
    bool      fromEnemy   = true;
};

// ── System ────────────────────────────────────────────────────────────────────
class EnemySystem
{
public:
    // Callback fired when a projectile hits the player
    std::function<void(Entity projectile, Entity player)> onProjectileHit;
    // Callback fired when an enemy touches the player
    std::function<void(Entity enemy,      Entity player)> onEnemyContact;
    // Callback when enemy dies (for power-up spawn, score, etc.)
    std::function<void(World& world, Entity enemy, glm::vec2 position)> onEnemyDeath;

    void update(World& world, const Tilemap& tilemap,
                Entity playerEntity, float dt);

    // Spawn helpers — call from GameplayState::buildLevel()
    static Entity spawnPatroller(World& world, glm::vec2 pos,
                                 float patrolLeft, float patrolRight,
                                 GLuint texID);
    static Entity spawnFlyer    (World& world, glm::vec2 pos,
                                 float hoverY, GLuint texID);
    static Entity spawnShooter  (World& world, glm::vec2 pos,
                                 GLuint texID, GLuint projTexID);
    static Entity spawnJumper   (World& world, glm::vec2 pos,
                                 GLuint texID);

private:
    void updatePatroller(World&, Entity, EnemyComponent&,
                         TransformComponent&, RigidbodyComponent&,
                         const Tilemap&, float dt);
    void updateFlyer    (World&, Entity, EnemyComponent&,
                         TransformComponent&, glm::vec2 playerPos, float dt);
    void updateShooter  (World&, Entity, EnemyComponent&,
                         TransformComponent&, glm::vec2 playerPos, float dt);
    void updateJumper   (World&, Entity, EnemyComponent&,
                         TransformComponent&, RigidbodyComponent&,
                         glm::vec2 playerPos, float dt);
    void updateProjectiles(World&, Entity playerEntity, float dt);
};
