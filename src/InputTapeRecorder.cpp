#include "InputTapeRecorder.h"

#include "Controller.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
enum class ReplayFormat
{
	Unknown,
	TickV2,
	LegacyTime
};

struct TickReplayEvent
{
	uint64_t tick = 0;
	std::string actionName;
	std::string eventType;
	bool active = false;
	size_t sequence = 0;
};

struct LegacyReplayEvent
{
	double timeSeconds = 0.0;
	std::string actionName;
	bool active = false;
	size_t sequence = 0;
};

bool gInitialized = false;
bool gRecordEnabled = false;
bool gReplayEnabled = false;
bool gLegacyReplayMode = false;

std::ofstream gRecordOut;
std::string gRecordPath;
std::string gReplayPath;

std::vector<TickReplayEvent> gTickReplayEvents;
std::vector<LegacyReplayEvent> gLegacyReplayEvents;
size_t gTickReplayCursor = 0;
size_t gLegacyReplayCursor = 0;

std::unordered_set<std::string> gReplayActions;
std::unordered_map<std::string, bool> gReplayActionState;

uint64_t gLoadedReplayEvents = 0;
uint64_t gConsumedReplayEvents = 0;
bool gWarnedLegacyReplay = false;

bool isTruthy(const char* value)
{
	if (!value || value[0] == '\0') {
		return false;
	}

	std::string lowered(value);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});

	return lowered == "1" ||
		lowered == "true" ||
		lowered == "yes" ||
		lowered == "on";
}

std::string trim(const std::string& input)
{
	size_t first = 0;
	while (first < input.size() && std::isspace((unsigned char)input[first])) {
		++first;
	}

	size_t last = input.size();
	while (last > first && std::isspace((unsigned char)input[last - 1])) {
		--last;
	}

	return input.substr(first, last - first);
}

std::string toLowerCopy(const std::string& value)
{
	std::string lowered(value);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
		return (char)std::tolower(c);
	});
	return lowered;
}

bool parseBoolToken(const std::string& token, bool fallback)
{
	const std::string lowered = toLowerCopy(trim(token));
	if (lowered.empty()) {
		return fallback;
	}

	if (lowered == "1" ||
		lowered == "true" ||
		lowered == "yes" ||
		lowered == "on" ||
		lowered == "down" ||
		lowered == "pressed") {
		return true;
	}

	if (lowered == "0" ||
		lowered == "false" ||
		lowered == "no" ||
		lowered == "off" ||
		lowered == "up" ||
		lowered == "released") {
		return false;
	}

	return fallback;
}

std::vector<std::string> splitCsvRow(const std::string& line)
{
	std::vector<std::string> tokens;
	std::stringstream stream(line);
	std::string token;
	while (std::getline(stream, token, ',')) {
		tokens.push_back(trim(token));
	}
	return tokens;
}

bool ensureParentDirectory(const std::string& path)
{
	if (path.empty()) {
		return false;
	}

	std::filesystem::path fsPath(path);
	const std::filesystem::path parent = fsPath.parent_path();
	if (parent.empty()) {
		return true;
	}

	std::error_code ec;
	std::filesystem::create_directories(parent, ec);
	return !ec;
}

bool parseUnsignedToken(const std::string& token, uint64_t& outValue)
{
	if (token.empty()) {
		return false;
	}

	char* endPtr = nullptr;
	unsigned long long parsedValue = std::strtoull(token.c_str(), &endPtr, 10);
	if (endPtr == token.c_str()) {
		return false;
	}

	while (endPtr && *endPtr != '\0') {
		if (!std::isspace((unsigned char)*endPtr)) {
			return false;
		}
		++endPtr;
	}

	outValue = (uint64_t)parsedValue;
	return true;
}

bool parseDoubleToken(const std::string& token, double& outValue)
{
	if (token.empty()) {
		return false;
	}

	char* endPtr = nullptr;
	const double parsedValue = std::strtod(token.c_str(), &endPtr);
	if (endPtr == token.c_str()) {
		return false;
	}

	while (endPtr && *endPtr != '\0') {
		if (!std::isspace((unsigned char)*endPtr)) {
			return false;
		}
		++endPtr;
	}

	outValue = parsedValue;
	return true;
}

ReplayFormat inferFormatFromTokens(const std::vector<std::string>& tokens)
{
	if (tokens.empty()) {
		return ReplayFormat::Unknown;
	}

	const std::string firstLower = toLowerCopy(tokens[0]);
	if (firstLower == "tick") {
		return ReplayFormat::TickV2;
	}
	if (firstLower == "time") {
		return ReplayFormat::LegacyTime;
	}

	if (tokens.size() >= 4) {
		uint64_t tickValue = 0;
		if (parseUnsignedToken(tokens[0], tickValue)) {
			return ReplayFormat::TickV2;
		}
	}

	if (tokens.size() >= 3) {
		double timeValue = 0.0;
		if (parseDoubleToken(tokens[0], timeValue)) {
			return ReplayFormat::LegacyTime;
		}
	}

	return ReplayFormat::Unknown;
}

void initializeReplayStateForAction(const std::string& actionName)
{
	if (actionName.empty()) {
		return;
	}

	gReplayActions.insert(actionName);
	if (gReplayActionState.find(actionName) == gReplayActionState.end()) {
		gReplayActionState[actionName] = false;
	}
}

void loadReplayFile(const std::string& path)
{
	std::ifstream replayIn(path);
	if (!replayIn.is_open()) {
		return;
	}

	ReplayFormat format = ReplayFormat::Unknown;
	std::string line;
	size_t sequence = 0;
	while (std::getline(replayIn, line)) {
		const std::string trimmedLine = trim(line);
		if (trimmedLine.empty() || trimmedLine[0] == '#') {
			continue;
		}

		std::vector<std::string> tokens = splitCsvRow(trimmedLine);
		if (tokens.empty()) {
			continue;
		}

		if (format == ReplayFormat::Unknown) {
			format = inferFormatFromTokens(tokens);
			const std::string firstLower = toLowerCopy(tokens[0]);
			if (firstLower == "tick" || firstLower == "time") {
				continue;
			}
		}

		if (format == ReplayFormat::TickV2) {
			if (tokens.size() < 4) {
				continue;
			}

			uint64_t tick = 0;
			if (!parseUnsignedToken(tokens[0], tick)) {
				continue;
			}

			const std::string actionName = tokens[1];
			if (actionName.empty()) {
				continue;
			}

			std::string eventType = tokens[2];
			if (eventType.empty()) {
				eventType = "EVT_KEYDOWN";
			}

			const bool active = parseBoolToken(tokens[3], false);

			TickReplayEvent event;
			event.tick = tick;
			event.actionName = actionName;
			event.eventType = eventType;
			event.active = active;
			event.sequence = sequence++;
			gTickReplayEvents.push_back(event);
			initializeReplayStateForAction(actionName);
			continue;
		}

		if (format == ReplayFormat::LegacyTime) {
			if (tokens.size() < 3) {
				continue;
			}

			double timeSeconds = 0.0;
			if (!parseDoubleToken(tokens[0], timeSeconds)) {
				continue;
			}

			const std::string actionName = tokens[1];
			if (actionName.empty()) {
				continue;
			}

			const bool active = parseBoolToken(tokens[2], false);
			LegacyReplayEvent event;
			event.timeSeconds = timeSeconds;
			event.actionName = actionName;
			event.active = active;
			event.sequence = sequence++;
			gLegacyReplayEvents.push_back(event);
			initializeReplayStateForAction(actionName);
		}
	}

	if (!gTickReplayEvents.empty()) {
		std::stable_sort(gTickReplayEvents.begin(), gTickReplayEvents.end(), [](const TickReplayEvent& a, const TickReplayEvent& b) {
			if (a.tick != b.tick) {
				return a.tick < b.tick;
			}
			if (a.actionName != b.actionName) {
				return a.actionName < b.actionName;
			}
			return a.sequence < b.sequence;
		});
	}

	if (!gLegacyReplayEvents.empty()) {
		gLegacyReplayMode = true;
		std::stable_sort(gLegacyReplayEvents.begin(), gLegacyReplayEvents.end(), [](const LegacyReplayEvent& a, const LegacyReplayEvent& b) {
			if (a.timeSeconds != b.timeSeconds) {
				return a.timeSeconds < b.timeSeconds;
			}
			if (a.actionName != b.actionName) {
				return a.actionName < b.actionName;
			}
			return a.sequence < b.sequence;
		});
	}

	gLoadedReplayEvents = (uint64_t)(gTickReplayEvents.size() + gLegacyReplayEvents.size());
}
}

namespace InputTapeRecorder
{
void initializeFromEnvironment(void)
{
	if (gInitialized) {
		return;
	}
	gInitialized = true;

	std::string owned;

	gRecordEnabled = isTruthy(System::getenv_platform("AUTO_INPUT_RECORD", owned));
	gReplayEnabled =
		isTruthy(System::getenv_platform("AUTO_INPUT_REPLAY", owned)) ||
		(System::getenv_platform("AUTO_INPUT_REPLAY_PATH", owned) && System::getenv_platform("AUTO_INPUT_REPLAY_PATH", owned)[0] != '\0');

	const char* recordPathValue = System::getenv_platform("AUTO_INPUT_RECORD_PATH", owned);
	gRecordPath = (recordPathValue && recordPathValue[0] != '\0') ?
		recordPathValue :
		"tmp/auto_input_events.csv";

	const char* replayPathValue = System::getenv_platform("AUTO_INPUT_REPLAY_PATH", owned);
	gReplayPath = (replayPathValue && replayPathValue[0] != '\0') ?
		replayPathValue :
		gRecordPath;

	if (gRecordEnabled && ensureParentDirectory(gRecordPath)) {
		gRecordOut.open(gRecordPath, std::ios::out | std::ios::trunc);
		if (gRecordOut.is_open()) {
			gRecordOut << "tick,action,event_type,active\n";
		}
	}

	if (gReplayEnabled) {
		loadReplayFile(gReplayPath);
	}
}

void shutdown(void)
{
	if (gRecordOut.is_open()) {
		gRecordOut.flush();
		gRecordOut.close();
	}

	gInitialized = false;
	gRecordEnabled = false;
	gReplayEnabled = false;
	gLegacyReplayMode = false;
	gTickReplayEvents.clear();
	gLegacyReplayEvents.clear();
	gReplayActions.clear();
	gReplayActionState.clear();
	gTickReplayCursor = 0;
	gLegacyReplayCursor = 0;
	gLoadedReplayEvents = 0;
	gConsumedReplayEvents = 0;
	gWarnedLegacyReplay = false;
}

bool isRecording(void)
{
	initializeFromEnvironment();
	return gRecordEnabled && gRecordOut.is_open();
}

bool isReplayEnabled(void)
{
	initializeFromEnvironment();
	return gReplayEnabled;
}

void onControllerTickStart(Controller* controller, uint64_t simulationTick, float controllerElapsedSeconds)
{
	(void)controller;

	initializeFromEnvironment();
	if (!gReplayEnabled) {
		return;
	}

	while (gTickReplayCursor < gTickReplayEvents.size() &&
		gTickReplayEvents[gTickReplayCursor].tick <= simulationTick) {
		const TickReplayEvent& event = gTickReplayEvents[gTickReplayCursor];
		gReplayActionState[event.actionName] = event.active;
		++gTickReplayCursor;
		++gConsumedReplayEvents;
	}

	while (gLegacyReplayCursor < gLegacyReplayEvents.size() &&
		gLegacyReplayEvents[gLegacyReplayCursor].timeSeconds <= (double)controllerElapsedSeconds) {
		if (!gWarnedLegacyReplay) {
			gWarnedLegacyReplay = true;
			std::printf("[InputTapeRecorder] Legacy time replay mode enabled for %s\n", gReplayPath.c_str());
		}
		const LegacyReplayEvent& event = gLegacyReplayEvents[gLegacyReplayCursor];
		gReplayActionState[event.actionName] = event.active;
		++gLegacyReplayCursor;
		++gConsumedReplayEvents;
	}
}

bool isReplayControlledAction(const std::string& actionName)
{
	initializeFromEnvironment();
	if (!gReplayEnabled || actionName.empty()) {
		return false;
	}
	return true;
}

bool getReplayActionState(const std::string& actionName, bool fallbackState)
{
	initializeFromEnvironment();
	if (!gReplayEnabled || actionName.empty()) {
		return fallbackState;
	}

	std::unordered_map<std::string, bool>::const_iterator stateItr = gReplayActionState.find(actionName);
	if (stateItr == gReplayActionState.end()) {
		return fallbackState;
	}
	return stateItr->second;
}

void recordActionSnapshot(uint64_t simulationTick, const std::string& actionName, bool active)
{
	(void)simulationTick;
	(void)actionName;
	(void)active;
}

void onControllerActionEvent(uint64_t simulationTick, float controllerElapsedSeconds, const std::string& actionName, const char* eventType, bool active)
{
	(void)controllerElapsedSeconds;

	if (!isRecording()) {
		return;
	}

	if (actionName.empty()) {
		return;
	}

	gRecordOut
		<< simulationTick << ","
		<< actionName << ","
		<< (eventType ? eventType : "") << ","
		<< (active ? 1 : 0) << "\n";
	gRecordOut.flush();
}

uint64_t getLoadedReplayEvents(void)
{
	initializeFromEnvironment();
	return gLoadedReplayEvents;
}

uint64_t getConsumedReplayEvents(void)
{
	initializeFromEnvironment();
	return gConsumedReplayEvents;
}
}
