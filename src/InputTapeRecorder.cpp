#include "InputTapeRecorder.h"

#include "StrUtils.h"
#include "System.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace
{
struct Change
{
	uint64_t tick = 0;
	std::string action;
	bool down = false;
};

const char* const kDefaultTapePath = "tmp/auto_input_events.csv";

bool gInitialized = false;
std::ofstream gRecordOut;
bool gReplaying = false;
std::vector<Change> gTape;
size_t gTapeCursor = 0;
std::unordered_map<std::string, bool> gReplayState;

std::string environmentPath(const char* name, const std::string& fallback)
{
	std::string owned;
	const char* value = System::getenv_platform(name, owned);
	return (value && value[0] != '\0') ? std::string(value) : fallback;
}

bool parseTick(const std::string& token, uint64_t& tick)
{
	char* end = nullptr;
	tick = std::strtoull(token.c_str(), &end, 10);
	return end != token.c_str() && *end == '\0';
}

void loadTape(const std::string& path)
{
	std::ifstream in(path);
	std::string line;
	while (std::getline(in, line)) {
		const std::string row = StrUtils::Trim(line);
		if (row.empty() || row[0] == '#') {
			continue;
		}
		const std::vector<std::string> fields = StrUtils::Split(row, ',', true);
		uint64_t tick = 0;
		if (fields.size() != 3 || !parseTick(fields[0], tick) || fields[1].empty()) {
			continue; // the header, or a malformed row
		}
		gTape.push_back({ tick, fields[1], fields[2] == "1" });
	}
	// Rows within a tick apply in file order.
	std::stable_sort(gTape.begin(), gTape.end(), [](const Change& a, const Change& b) {
		return a.tick < b.tick;
	});
	std::printf("[InputTapeRecorder] Replaying %zu changes from %s\n", gTape.size(), path.c_str());
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
	const std::string recordPath = environmentPath("AUTO_INPUT_RECORD_PATH", kDefaultTapePath);
	if (StrUtils::IsTruthy(System::getenv_platform("AUTO_INPUT_RECORD", owned))) {
		std::error_code ec;
		const std::filesystem::path parent = std::filesystem::path(recordPath).parent_path();
		if (!parent.empty()) {
			std::filesystem::create_directories(parent, ec);
		}
		gRecordOut.open(recordPath, std::ios::out | std::ios::trunc);
		if (gRecordOut.is_open()) {
			gRecordOut << "tick,action,down\n";
		}
	}

	const char* replayPath = System::getenv_platform("AUTO_INPUT_REPLAY_PATH", owned);
	gReplaying = StrUtils::IsTruthy(System::getenv_platform("AUTO_INPUT_REPLAY", owned)) ||
		(replayPath && replayPath[0] != '\0');
	if (gReplaying) {
		loadTape(environmentPath("AUTO_INPUT_REPLAY_PATH", recordPath));
	}
}

void shutdown(void)
{
	if (gRecordOut.is_open()) {
		gRecordOut.close();
	}
	gInitialized = false;
	gReplaying = false;
	gTape.clear();
	gTapeCursor = 0;
	gReplayState.clear();
}

bool isRecording(void)
{
	initializeFromEnvironment();
	return gRecordOut.is_open();
}

bool isReplaying(void)
{
	initializeFromEnvironment();
	return gReplaying;
}

void beginTick(uint64_t tick)
{
	while (gTapeCursor < gTape.size() && gTape[gTapeCursor].tick <= tick) {
		const Change& change = gTape[gTapeCursor++];
		gReplayState[change.action] = change.down;
	}
}

bool replayState(const std::string& action)
{
	const auto state = gReplayState.find(action);
	return state != gReplayState.end() && state->second;
}

void recordChange(uint64_t tick, const std::string& action, bool down)
{
	if (!isRecording()) {
		return;
	}
	// Flushed per row so a tape survives the app being killed.
	gRecordOut << tick << ',' << action << ',' << (down ? 1 : 0) << '\n';
	gRecordOut.flush();
}
}
