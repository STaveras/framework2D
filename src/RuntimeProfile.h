#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <ostream>

// Single-threaded game-loop instrumentation. No per-sample allocations or I/O.
// Timings are inclusive; nested regions must not be added together.
namespace RuntimeProfile {
enum class Region { GameTick, Input, ObjectUpdate, Collision, BoarUpdate, BoarSupport,
    CharacterSupport, Render, SpatialQuery, SpatialRebuild, Count };
enum class Counter { BoarCandidates, CharacterCandidates, CollisionCandidates,
    SpatialCandidates, SpatialRebuilds, Count };
struct Sample { std::uint64_t calls = 0; double milliseconds = 0; };
inline bool active = false;
inline std::array<Sample, static_cast<size_t>(Region::Count)> samples{};
inline std::array<std::uint64_t, static_cast<size_t>(Counter::Count)> counters{};
using Clock = std::chrono::steady_clock;

class Scope {
    Region region;
    bool recording;
    Clock::time_point start;
public:
    explicit Scope(Region value) : region(value), recording(active) {
        if (recording) start = Clock::now();
    }
    ~Scope() {
        if (!recording) return;
        auto& sample = samples[static_cast<size_t>(region)];
        ++sample.calls;
        sample.milliseconds += std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
inline void count(Counter counter, std::uint64_t amount = 1) {
    if (active) counters[static_cast<size_t>(counter)] += amount;
}
inline void report(std::ostream& out) {
    constexpr const char* regions[] = {"game_tick", "input", "object_update", "collision",
        "boar_update", "boar_support", "character_support", "render", "spatial_query", "spatial_rebuild"};
    constexpr const char* names[] = {"boar_candidates", "character_candidates", "collision_candidates",
        "spatial_candidates", "spatial_rebuilds"};
    const auto ticks = samples[static_cast<size_t>(Region::GameTick)].calls;
    out << "PROFILE ticks=" << ticks << " timings=inclusive\n";
    for (size_t i = 0; i < samples.size(); ++i) {
        if (!samples[i].calls) continue;
        out << "PROFILE region=" << regions[i] << " calls=" << samples[i].calls
            << " total_ms=" << samples[i].milliseconds
            << " ms_per_tick=" << (ticks ? samples[i].milliseconds / ticks : 0.0) << '\n';
    }
    for (size_t i = 0; i < counters.size(); ++i) {
        out << "PROFILE counter=" << names[i] << " total=" << counters[i]
            << " per_tick=" << (ticks ? static_cast<double>(counters[i]) / ticks : 0.0) << '\n';
    }
}
}
