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

// ── World ─────────────────────────────────────────────────────────────────────
/**
 * World
 *
 * Creates and destroys entities, attaches/detaches components, and
 * provides typed views for system iteration.
 *
 *   World world;
 *
 *   Entity player = world.create();
 *   world.add<TransformComponent>(player, { {100, 400} });
 *   world.add<RigidbodyComponent>(player);
 *
 *   // In a system:
 *   for (Entity e : world.view<TransformComponent, RigidbodyComponent>())
 *   {
 *       auto& tf = world.get<TransformComponent>(e);
 *       auto& rb = world.get<RigidbodyComponent>(e);
 *       ...
 *   }
 *
 *   world.destroy(player);   // removes all components immediately
 *   world.clear();           // nuke everything (e.g. on level unload)
 */
class World
{
public:
    // ── Entity lifecycle ─────────────────────────────────────────────────────

    Entity create()
    {
        Entity e = m_nextId++;
        m_alive.push_back(e);
        return e;
    }

    /** Remove all components and mark the entity dead immediately. */
    void destroy(Entity e)
    {
        for (auto& [ti, pool] : m_pools)
            pool->remove(e);

        m_alive.erase(
            std::remove(m_alive.begin(), m_alive.end(), e),
            m_alive.end());
    }

    /** Destroy every entity and reset the ID counter. */
    void clear()
    {
        m_pools.clear();
        m_alive.clear();
        m_nextId = 1;
    }

    size_t entityCount() const { return m_alive.size(); }

