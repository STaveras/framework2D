#pragma once

#include <cstdint>
#include <string>

class Controller;

namespace InputTapeRecorder
{
void initializeFromEnvironment(void);
void shutdown(void);

bool isRecording(void);
bool isReplayEnabled(void);

void onControllerTickStart(Controller* controller, uint64_t simulationTick, float controllerElapsedSeconds);
bool isReplayControlledAction(const std::string& actionName);
bool getReplayActionState(const std::string& actionName, bool fallbackState);

void recordActionSnapshot(uint64_t simulationTick, const std::string& actionName, bool active);
void onControllerActionEvent(uint64_t simulationTick, float controllerElapsedSeconds, const std::string& actionName, const char* eventType, bool active);

uint64_t getLoadedReplayEvents(void);
uint64_t getConsumedReplayEvents(void);
}

