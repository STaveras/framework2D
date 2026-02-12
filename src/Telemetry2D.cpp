#include "Telemetry2D.h"

#include "System.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iomanip>

namespace
{
constexpr float kAutoSummaryIntervalSeconds = 1.0f;

bool isTruthyEnvValue(const char* value)
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

std::string getEnvOrDefault(const char* name, const char* fallback)
{
	std::string owned;
	const char* envValue = System::getenv_platform(name, owned);
	if (envValue && envValue[0] != '\0') {
		return std::string(envValue);
	}
	return std::string(fallback ? fallback : "");
}

void ensureParentDirectory(const std::string& path)
{
	if (path.empty()) {
		return;
	}

	std::filesystem::path fsPath(path);
	const std::filesystem::path parent = fsPath.parent_path();
	if (parent.empty()) {
		return;
	}

	std::error_code ec;
	std::filesystem::create_directories(parent, ec);
}

} // namespace

namespace Telemetry2D
{

void initialize(Runtime& runtime, const char* defaultPath)
{
	if (runtime.initialized) {
		return;
	}

	runtime.initialized = true;
	runtime.enabled =
		isTruthyEnvValue(std::getenv("AUTO_MOVEMENT_TELEMETRY")) ||
		isTruthyEnvValue(std::getenv("AUTO_SLOPE_TELEMETRY")) ||
		isTruthyEnvValue(std::getenv("AUTO_SLOPE_TEST"));
	runtime.telemetryEnabled = runtime.enabled;

	std::string path = getEnvOrDefault("AUTO_MOVEMENT_LOG_PATH", "");
	if (path.empty()) {
		path = getEnvOrDefault("AUTO_SLOPE_LOG_PATH", defaultPath ? defaultPath : "tmp/auto_slope_telemetry.csv");
	}
	runtime.telemetryPath = path;

	if (runtime.telemetryEnabled) {
		ensureParentDirectory(runtime.telemetryPath);
		runtime.telemetryOut.open(runtime.telemetryPath, std::ios::out | std::ios::trunc);
		if (runtime.telemetryOut.is_open()) {
			runtime.telemetryOut
				<< "time,dt,replay_tick,state,pos_x,pos_y,vel_x,vel_y,intent,"
				<< "dx_signed,dx_abs,net_dx_accum,dx_speed,path_speed,wall_correction_x,"
				<< "support_source,contact_class,ground_tile,ground_shape,grounded,contacts,dropouts,jitter,"
				<< "square_dx_avg,polygon_dx_avg,square_path_avg,polygon_path_avg\n";
		}
	}
}

void shutdown(Runtime& runtime)
{
	if (runtime.telemetryOut.is_open()) {
		runtime.telemetryOut.flush();
		runtime.telemetryOut.close();
	}
}

bool isEnabled(const Runtime& runtime)
{
	return runtime.enabled && runtime.telemetryEnabled && runtime.telemetryOut.is_open();
}

void appendSample(Runtime& runtime, const Sample& sample)
{
	if (!isEnabled(runtime)) {
		return;
	}

	runtime.telemetryOut << std::fixed << std::setprecision(6)
		<< sample.time << ","
		<< sample.dt << ","
		<< sample.replayTick << ","
		<< (sample.stateName ? sample.stateName : "(null)") << ","
		<< sample.position.x << ","
		<< sample.position.y << ","
		<< sample.velocity.x << ","
		<< sample.velocity.y << ","
		<< sample.intent << ","
		<< sample.dxSigned << ","
		<< sample.dxAbs << ","
		<< sample.netDxAccum << ","
		<< sample.dxSpeed << ","
		<< sample.pathSpeed << ","
		<< sample.wallCorrectionX << ","
		<< (sample.supportSource ? sample.supportSource : "none") << ","
		<< (sample.contactClass ? sample.contactClass : "unknown") << ","
		<< sample.groundTile << ","
		<< (sample.groundShape ? sample.groundShape : "none") << ","
		<< sample.grounded << ","
		<< sample.contacts << ","
		<< sample.dropouts << ","
		<< sample.jitter << ","
		<< sample.squareDxAvg << ","
		<< sample.polygonDxAvg << ","
		<< sample.squarePathAvg << ","
		<< sample.polygonPathAvg << "\n";

	runtime.summaryTimerSeconds += sample.dt;
	if (runtime.summaryTimerSeconds >= kAutoSummaryIntervalSeconds) {
		runtime.summaryTimerSeconds = 0.0;
		printf(
			"[AUTO_SUMMARY] t=%.2f square_dx=%.2f polygon_dx=%.2f square_path=%.2f polygon_path=%.2f jitter_max=%.2f dropouts=%d\n",
			(float)sample.time,
			(float)sample.squareDxAvg,
			(float)sample.polygonDxAvg,
			(float)sample.squarePathAvg,
			(float)sample.polygonPathAvg,
			(float)runtime.maxHorizontalJitter,
			sample.dropouts);
	}

	runtime.telemetryOut.flush();
}

} // namespace Telemetry2D
