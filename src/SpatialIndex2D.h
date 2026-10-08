#pragma once

#include "Types.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

class GameObject;
class Collidable;

// A small, owner-independent broad phase for static geometry. The index owns
// no GameObjects; its caller files each static object with its rank in the
// caller's iteration order, and re-files objects whose geometry changed.
// Dynamic objects deliberately stay out of this grid: their bounds are
// queried live because they move frequently.
class SpatialIndex2D
{
public:
    static constexpr float kCellSize = 128.0f;

    // A filed object and its caller-assigned rank, so results can be ordered
    // without a lookup per comparison.
    struct Entry
    {
        size_t order = 0;
        GameObject* object = nullptr;
    };

    // Returns a complete finite AABB only when every non-null member of a
    // compound collider is understood. Unsupported or malformed geometry
    // returns false so callers can retain it as a conservative candidate.
    static bool tryGetConservativeBounds(
        const Collidable* collidable,
        vector2& outMin,
        vector2& outMax);

    void clear(void);

    // Files object under its current collider bounds, replacing any earlier
    // filing. Objects that are not static or have no collider are dropped.
    void update(GameObject* object, size_t order);

    // Appends every static object that may overlap [min, max]; an object
    // spanning several cells is appended once per cell.
    void collectStaticCandidates(
        const vector2& min,
        const vector2& max,
        std::vector<Entry>& out) const;

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

    // Where an object is filed: a cell range, or the unbounded list.
    struct Filing
    {
        size_t order = 0;
        bool unbounded = false;
        Cell minCell;
        Cell maxCell;
    };

    void remove(GameObject* object);

    std::unordered_map<Cell, std::vector<Entry>, CellHash> _staticCells;
    std::unordered_map<GameObject*, Filing> _filings;
    std::vector<Entry> _unboundedStatic;
};
