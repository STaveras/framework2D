#pragma once

#include "RuntimeProfile.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif
#endif

// Late input sampling ("frame delay"). With VSync on, a conventional loop polls
// input right after the swap returns, so input waits a whole refresh before the
// work that uses it even starts. The pacer instead sleeps until
//   next vblank - (recent worst update+render time + margin)
// and only then polls input, so the sampled input is as fresh as possible when
// the frame scans out.
//
// The vblank is predicted from the refresh period and the time blocking swaps
// return (a small phase-locked loop). When the driver's swap does not block
// (headless/Xvfb, compositors that queue), the prediction free-runs on the
// refresh period. simulateVsync replaces the display with a virtual one whose
// swap blocks until the next predicted vblank, so paced and unpaced runs can be
// compared on machines without a real vsync.
//
// Single-threaded, no per-frame allocations.
namespace FramePacer {
using Clock = std::chrono::steady_clock;
using Ms = std::chrono::duration<double, std::milli>;

struct Config {
	bool enabled = false;       // wait before polling input
	bool simulateVsync = false; // present to a virtual display that blocks until its vblank
	double refreshHz = 60.0;
	double marginMs = 2.0;      // safety margin added to the measured work time
};

namespace detail {
constexpr size_t kWorkHistory = 32;
inline Config config;
inline double periodMs = 1000.0 / 60.0;
inline Clock::time_point anchor{};          // most recent (estimated) vblank
inline bool hasAnchor = false;
inline Clock::time_point wake{};            // when this frame's work started
inline Clock::time_point inputSampled{};
inline Clock::time_point aimedVblank{};     // vblank this frame was paced for
inline Clock::time_point presentStart{};
inline bool frameOpen = false;
inline std::array<double, kWorkHistory> work{};
inline std::array<double, kWorkHistory> swap{}; // swap call duration: overhead plus any vblank wait
inline size_t workCount = 0, workNext = 0;
inline double extraMarginMs = 0.0;         // grows after a missed vblank, decays back
inline std::vector<double> latencies;      // input sampled -> vblank, ms (profiling only)
inline double waitTotalMs = 0.0;
inline std::uint64_t missed = 0, frames = 0;

inline Clock::time_point add(Clock::time_point t, double ms) {
	return t + std::chrono::duration_cast<Clock::duration>(Ms(ms));
}

// First predicted vblank at or after t.
inline Clock::time_point vblankAtOrAfter(Clock::time_point t) {
	if (!hasAnchor) return t;
	const double since = Ms(t - anchor).count();
	const double periods = std::max(0.0, std::ceil(since / periodMs - 1e-6));
	return add(anchor, periods * periodMs);
}

inline double budgetMs() {
	// The fastest recent swap approximates the swap's own cost; slower ones were
	// waiting for the vblank, which the budget must not include.
	double worst = 0.0, swapCost = workCount ? swap[0] : 0.0;
	for (size_t i = 0; i < workCount; ++i) {
		worst = std::max(worst, work[i]);
		swapCost = std::min(swapCost, swap[i]);
	}
	// Never budget more than a refresh: at worst the frame starts right after the
	// previous vblank, which is what an unpaced VSync loop does anyway.
	return std::min(worst + swapCost + config.marginMs + extraMarginMs, periodMs * 0.95);
}

// Coarse sleep, then yield-spin the last stretch; OS sleeps overshoot.
inline void sleepUntil(Clock::time_point target) {
	constexpr double kSpinMs = 1.5;
	for (;;) {
		const double remaining = Ms(target - Clock::now()).count();
		if (remaining <= 0.0) return;
		if (remaining > kSpinMs) std::this_thread::sleep_for(Ms(remaining - kSpinMs));
		else std::this_thread::yield();
	}
}
} // namespace detail

inline void configure(const Config& config) {
	using namespace detail;
	detail::config = config;
	periodMs = 1000.0 / std::clamp(config.refreshHz, 1.0, 1000.0);
	hasAnchor = false;
	workCount = workNext = 0;
	extraMarginMs = 0.0;
#ifdef _WIN32
	// Default Windows timer resolution (~15.6 ms) would make every wait overshoot.
	if (config.enabled || config.simulateVsync) timeBeginPeriod(1);
#endif
}

inline bool enabled() { return detail::config.enabled; }
// Renderers that can force their swap to block (e.g. glFinish after SwapBuffers)
// should do so when this is true, so the swap really returns at the vblank.
inline bool wantsHardSync() { return detail::config.enabled; }
inline double refreshPeriodMs() { return detail::periodMs; }

// Call at the top of the frame, before polling window events.
inline void waitForInputDeadline() {
	using namespace detail;
	const Clock::time_point now = Clock::now();
	if (config.enabled && hasAnchor) {
		// Aim for the earliest vblank we can still make with the current budget.
		const double budget = budgetMs();
		Clock::time_point target = vblankAtOrAfter(add(now, budget));
		const Clock::time_point deadline = add(target, -budget);
		if (deadline > now) sleepUntil(deadline);
		aimedVblank = target;
		if (RuntimeProfile::active) waitTotalMs += Ms(Clock::now() - now).count();
	}
	else {
		aimedVblank = Clock::time_point{};
	}
	wake = Clock::now();
	frameOpen = true;
}

// Call right after window events (input) have been polled.
inline void markInputSampled() { detail::inputSampled = Clock::now(); }

// Renderers bracket their swap/present call with these.
inline void presentBegin() { detail::presentStart = Clock::now(); }

inline void presentEnd() {
	using namespace detail;
	Clock::time_point end = Clock::now();
	const double swapMs = Ms(end - presentStart).count();
	if (!hasAnchor) {
		anchor = end;
		hasAnchor = true;
	}

	Clock::time_point vblank;
	if (config.simulateVsync) {
		// Virtual display: the frame scans out at the first vblank after the swap.
		vblank = vblankAtOrAfter(end);
		sleepUntil(vblank);
	}
	else if (swapMs > 0.5) {
		// The swap blocked, so it returned at (just after) a real vblank. Nudge the
		// predicted phase toward it; resynchronize outright if it is far off.
		const double since = Ms(end - anchor).count();
		const Clock::time_point nearest = add(anchor, std::round(since / periodMs) * periodMs);
		const double error = Ms(end - nearest).count();
		vblank = (std::abs(error) < periodMs * 0.25) ? add(nearest, error * 0.5) : end;
	}
	else {
		// The swap was queued; the best guess is the next predicted vblank.
		vblank = vblankAtOrAfter(end);
	}
	anchor = vblank;

	if (frameOpen) {
		work[workNext] = Ms(presentStart - wake).count();
		swap[workNext] = swapMs;
		workNext = (workNext + 1) % kWorkHistory;
		workCount = std::min(workCount + 1, kWorkHistory);

		// Work submitted before the swap can still finish on the GPU after it, which
		// the CPU-side work history cannot see. Grow the margin by the overshoot when
		// a vblank is missed, then let it decay over a few dozen frames.
		const double lateMs = (config.enabled && aimedVblank != Clock::time_point{})
			? Ms(vblank - aimedVblank).count() : 0.0;
		const bool missedVblank = lateMs > periodMs * 0.5;
		if (missedVblank) extraMarginMs = std::min(extraMarginMs + 0.5 + Ms(end - aimedVblank).count(), periodMs * 0.5);
		else extraMarginMs = std::max(0.0, extraMarginMs - 0.05);

		if (RuntimeProfile::active) {
			++frames;
			if (missedVblank) ++missed;
			if (latencies.capacity() == latencies.size()) latencies.reserve(latencies.size() * 2 + 4096);
			latencies.push_back(Ms(vblank - inputSampled).count());
		}
	}
	frameOpen = false;
}

inline void report(std::ostream& out) {
	using namespace detail;
	if (latencies.empty()) return;
	std::vector<double> sorted = latencies;
	std::sort(sorted.begin(), sorted.end());
	double total = 0.0;
	for (double ms : sorted) total += ms;
	out << "PACING enabled=" << (config.enabled ? 1 : 0)
	    << " simulated_vsync=" << (config.simulateVsync ? 1 : 0)
	    << " refresh_hz=" << 1000.0 / periodMs
	    << " frames=" << frames
	    << " input_to_vblank_mean_ms=" << total / sorted.size()
	    << " p95_ms=" << sorted[(sorted.size() - 1) * 95 / 100]
	    << " p99_ms=" << sorted[(sorted.size() - 1) * 99 / 100]
	    << " wait_ms_per_frame=" << (frames ? waitTotalMs / frames : 0.0)
	    << " missed_vblanks=" << missed << '\n';
}
} // namespace FramePacer
