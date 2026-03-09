#pragma once

//
// Components are plain data structs — no logic, no virtual functions.
// Systems (PhysicsWorld, AnimationSystem, etc.) own the behaviour.

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <unordered_map>




// ── TransformComponent ───────────────────────────────────────────────────────
// World-space position, uniform scale, and rotation.
// Scale is applied per-axis so sprites can be flipped: scale.x = -1 mirrors X.
struct TransformComponent
{
    glm::vec2 position = { 0.f, 0.f };
    glm::vec2 scale    = { 1.f, 1.f };
    float     rotation = 0.f;           // degrees, clockwise
};

// ── SpriteComponent ──────────────────────────────────────────────────────────
// Describes one textured quad rendered via Renderer2D.
// size is in pixels (world units). layer controls draw order (higher = front).
struct SpriteComponent
{
    GLuint    textureID = 0;
    glm::vec2 size      = { 32.f, 32.f };
    glm::vec4 tint      = { 1.f, 1.f, 1.f, 1.f };
    glm::vec2 uvMin     = { 0.f, 0.f };
    glm::vec2 uvMax     = { 1.f, 1.f };
    int       layer     = 0;            // higher = drawn on top
};

// ── RigidbodyComponent ───────────────────────────────────────────────────────
// Drives kinematic motion. PhysicsWorld reads/writes velocity and onGround.
struct RigidbodyComponent
{
    glm::vec2 velocity     = { 0.f, 0.f };
    float     gravityScale = 1.f;
    bool      onGround     = false;
    bool      useGravity   = true;
};

// ── BoxColliderComponent ─────────────────────────────────────────────────────
// Axis-aligned bounding box in LOCAL space (relative to TransformComponent).
// isTrigger: collisions are detected but not resolved (overlap callbacks only).
struct BoxColliderComponent
{
    glm::vec2 offset    = { 0.f, 0.f };   // local offset from transform.position
    glm::vec2 size      = { 32.f, 32.f };
    bool      isTrigger = false;
};

