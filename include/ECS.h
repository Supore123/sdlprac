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

