#pragma once

#include "Types.h"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class GameObject;
class Collidable;

// A small, owner-independent broad phase for static geometry. The index owns
// no GameObjects and is rebuilt from an ObjectManager's map when its caller's
// static revision changes. Dynamic objects deliberately stay out of this grid:
// their bounds are queried live because they move frequently.
class SpatialIndex2D
{
public:
    static constexpr float kCellSize = 128.0f;

    // Returns a complete finite AABB only when every non-null member of a
    // compound collider is understood. Unsupported or malformed geometry
    // returns false so callers can retain it as a conservative candidate.
    static bool tryGetConservativeBounds(
        const Collidable* collidable,
        vector2& outMin,
        vector2& outMax);

    void rebuild(const std::map<std::string, GameObject*>& objects);

    void collectStaticCandidates(
        const vector2& min,
        const vector2& max,
        std::unordered_set<GameObject*>& out) const;

private:
    struct Cell
    {
        int x = 0;
        int y = 0;

        bool operator==(const Cell& rhs) const { return x == rhs.x && y == rhs.y; }
    };

    struct CellHash
    {
        size_t operator()(const Cell& cell) const
        {
            const uint64_t x = static_cast<uint32_t>(cell.x);
            const uint64_t y = static_cast<uint32_t>(cell.y);
            return static_cast<size_t>((x * 0x9E3779B185EBCA87ull) ^ (y + 0x517CC1B727220A95ull + (x << 6) + (x >> 2)));
        }
    };

    std::unordered_map<Cell, std::vector<GameObject*>, CellHash> _staticCells;
    std::vector<GameObject*> _allStatic;
    std::vector<GameObject*> _unboundedStatic;
};
