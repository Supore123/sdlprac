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

    // ── Component operations ─────────────────────────────────────────────────

    /** Add (or overwrite) a component on entity e. Returns a ref to it. */
    template<typename T>
    T& add(Entity e, T component = T{})
    {
        return pool<T>().add(e, std::move(component));
    }

    /** Get a reference to an existing component. Throws if not present. */
    template<typename T>
    T& get(Entity e)
    {
        return pool<T>().get(e);
    }

    /** Returns true if entity has component T. */
    template<typename T>
    bool has(Entity e) const
    {
        auto it = m_pools.find(std::type_index(typeid(T)));
        return it != m_pools.end() && it->second->has(e);
    }

    /** Remove component T from entity e (no-op if absent). */
    template<typename T>
    void remove(Entity e)
    {
        auto it = m_pools.find(std::type_index(typeid(T)));
        if (it != m_pools.end())
            it->second->remove(e);
    }

    // ── Views ────────────────────────────────────────────────────────────────

    /**
     * Returns all live entities that have ALL of the listed component types.
     *
     *   for (Entity e : world.view<Transform, Rigidbody, Collider>()) { ... }
     */
    template<typename T, typename... Rest>
    std::vector<Entity> view()
    {
        std::vector<Entity> result;
        result.reserve(m_alive.size());
        for (Entity e : m_alive)
            if (has<T>(e) && (has<Rest>(e) && ...))
                result.push_back(e);
        return result;
    }

    /** All live entities, regardless of components. */
    const std::vector<Entity>& all() const { return m_alive; }

private:
    Entity                   m_nextId = 1;
    std::vector<Entity>      m_alive;
    std::unordered_map<std::type_index,
                       std::unique_ptr<IComponentPool>> m_pools;

    template<typename T>
    ComponentPool<T>& pool()
    {
        auto key = std::type_index(typeid(T));
        auto it  = m_pools.find(key);
        if (it == m_pools.end())
        {
            m_pools[key] = std::make_unique<ComponentPool<T>>();
            it = m_pools.find(key);
        }
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }
};

