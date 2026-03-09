#pragma once

//
// Design: unordered_map<Entity, T> per component type.
// Not the fastest possible (cache-miss on random access), but dead simple to
// use correctly and fast enough for a 2D platformer with hundreds of entities.
// If profiling shows ECS as a hotspot, swap ComponentPool internals for a
// packed array (dense-set) without changing any call sites.

#include <cstdint>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <memory>
#include <vector>
#include <algorithm>




// ── Entity ───────────────────────────────────────────────────────────────────
using Entity = uint32_t;
constexpr Entity NULL_ENTITY = 0;   // sentinel "no entity"

// ── Internal type-erased pool base ───────────────────────────────────────────
struct IComponentPool
{
    virtual ~IComponentPool() = default;
    virtual void remove(Entity e)      = 0;
    virtual bool has   (Entity e) const = 0;
};

// ── Typed pool ───────────────────────────────────────────────────────────────
template<typename T>
struct ComponentPool final : IComponentPool
{
    std::unordered_map<Entity, T> data;

    T& add(Entity e, T component)
    {
        data[e] = std::move(component);
        return data[e];
    }

    T& get(Entity e)             { return data.at(e); }
    void remove(Entity e) override { data.erase(e); }
    bool has(Entity e) const override { return data.count(e) != 0; }
};

