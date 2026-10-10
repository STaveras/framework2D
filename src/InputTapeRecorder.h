// InputTapeRecorder.h
// Records and replays input as action state changes per simulation tick, so a
// session replays deterministically:
//   AUTO_INPUT_RECORD=1 [AUTO_INPUT_RECORD_PATH=tmp/auto_input_events.csv]
//   AUTO_INPUT_REPLAY=1 or AUTO_INPUT_REPLAY_PATH=<tape>
// A tape is CSV with a "tick,action,down" header and one row each time an
// action is pressed (1) or released (0). During a replay every action follows
// the tape (released until its first row) and the input devices are ignored.
// InputMap::update() drives both.

#pragma once

#include <cstdint>
#include <string>

namespace InputTapeRecorder
{
void initializeFromEnvironment(void);
void shutdown(void);

bool isRecording(void);
bool isReplaying(void);

// Apply the tape's changes up to and including this tick.
void beginTick(uint64_t tick);
// An action's state on the tape as of the last beginTick().
bool replayState(const std::string& action);

// Write a change to the tape being recorded.
void recordChange(uint64_t tick, const std::string& action, bool down);
}
