#pragma once

#include "Types.h"

#include <cstdint>
#include <fstream>
#include <string>

namespace Telemetry2D
{
struct Runtime
{
	bool initialized = false;
	bool enabled = false;
	bool telemetryEnabled = false;
	double elapsedSeconds = 0.0;
	double summaryTimerSeconds = 0.0;
	double previousTelemetryTime = 0.0;
	bool hasPreviousTelemetry = false;
	vector2 previousTelemetryPosition = vector2(0.0f, 0.0f);
	std::ofstream telemetryOut;
	std::string telemetryPath;
	double squareDxSpeedSum = 0.0;
	double squarePathSpeedSum = 0.0;
	size_t squareSampleCount = 0;
	double polygonDxSpeedSum = 0.0;
	double polygonPathSpeedSum = 0.0;
	size_t polygonSampleCount = 0;
	double maxHorizontalJitter = 0.0;
	double netDxAccum = 0.0;
	int groundDropouts = 0;
	bool previousGrounded = false;
};

struct Sample
{
	double time = 0.0;
	double dt = 0.0;
	uint64_t replayTick = 0;
	const char* stateName = "";
	vector2 position = vector2(0.0f, 0.0f);
	vector2 velocity = vector2(0.0f, 0.0f);
	int intent = 0;
	double dxSigned = 0.0;
	double dxAbs = 0.0;
	double netDxAccum = 0.0;
	double dxSpeed = 0.0;
	double pathSpeed = 0.0;
	double wallCorrectionX = 0.0;
	const char* supportSource = "none";
	const char* contactClass = "unknown";
	int groundTile = -1;
	const char* groundShape = "none";
	int grounded = 0;
	size_t contacts = 0;
	int dropouts = 0;
	double jitter = 0.0;
	double squareDxAvg = 0.0;
	double polygonDxAvg = 0.0;
	double squarePathAvg = 0.0;
	double polygonPathAvg = 0.0;
};

void initialize(Runtime& runtime, const char* defaultPath);
void shutdown(Runtime& runtime);
bool isEnabled(const Runtime& runtime);
void appendSample(Runtime& runtime, const Sample& sample);

} // namespace Telemetry2D
