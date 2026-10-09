#pragma once

#include "Types.h"

#include <cstdint>
#include <vector>

class Renderable;

// Culling groups for world-space render lists. The renderer splits each list into
// runs of consecutive sprites ("chunks") and keeps every chunk's bounds, so a whole
// off-screen chunk is skipped without visiting its sprites. A sprite records the id
// of its chunk, and its transform setters mark that chunk to be measured again.
namespace RenderCulling {

struct ListCache;

struct Chunk
{
	ListCache* owner = nullptr; // null while the id is free
	std::uint32_t segment = 0;  // index into owner->segments
	bool dirty = false;
};

// Indexed by chunk id; id 0 means "in no chunk". Never destroyed, so renderables
// and lists torn down during static destruction can still reach it.
inline std::vector<Chunk>& chunkTable(void)
{
	static std::vector<Chunk>* table = new std::vector<Chunk>(1);
	return *table;
}

inline std::vector<std::uint32_t>& freeChunkIds(void)
{
	static std::vector<std::uint32_t>* ids = new std::vector<std::uint32_t>();
	return *ids;
}

inline void markDirty(std::uint32_t id)
{
	if (id) {
		chunkTable()[id].dirty = true;
	}
}

// A run of a list's items, in list order. A chunk run holds only sprites and has
// bounds; any other run is a single item visited every frame.
struct Segment
{
	std::uint32_t begin = 0;
	std::uint32_t end = 0;
	std::uint32_t chunk = 0;
	vector2 min = vector2(0.0f, 0.0f);
	vector2 max = vector2(0.0f, 0.0f);
};

// One render list's grouping, rebuilt when the list's revision changes.
struct ListCache
{
	bool built = false;
	std::uint64_t revision = 0;
	std::vector<Renderable*> items;
	std::vector<Segment> segments;

	ListCache(void) = default;
	ListCache(const ListCache&) = delete;
	ListCache& operator=(const ListCache&) = delete;
	~ListCache(void) { release(); }

	std::uint32_t allocateChunk(std::uint32_t segment)
	{
		std::vector<Chunk>& table = chunkTable();
		std::vector<std::uint32_t>& freeIds = freeChunkIds();
		std::uint32_t id = 0;
		if (!freeIds.empty()) {
			id = freeIds.back();
			freeIds.pop_back();
		}
		else {
			id = static_cast<std::uint32_t>(table.size());
			table.emplace_back();
		}
		table[id] = Chunk{ this, segment, false };
		return id;
	}

	// Frees this list's chunk ids. Items may already be deleted, so they are not
	// touched: an item still holding a freed id is caught by reportsToChunk().
	void release(void)
	{
		std::vector<Chunk>& table = chunkTable();
		for (const Segment& segment : segments) {
			if (segment.chunk) {
				table[segment.chunk] = Chunk{};
				freeChunkIds().push_back(segment.chunk);
			}
		}
		segments.clear();
		items.clear();
		built = false;
	}
};

// Whether a chunk that is still in use holds this renderable under this id. Only
// pointers are compared, so stale ids and deleted members are safe to check.
inline bool reportsToChunk(const Renderable* renderable, std::uint32_t id)
{
	if (!id) {
		return false;
	}
	const Chunk& chunk = chunkTable()[id];
	if (!chunk.owner || chunk.segment >= chunk.owner->segments.size()) {
		return false;
	}
	const Segment& segment = chunk.owner->segments[chunk.segment];
	for (std::uint32_t i = segment.begin; i < segment.end; ++i) {
		if (chunk.owner->items[i] == renderable) {
			return true;
		}
	}
	return false;
}

} // namespace RenderCulling
