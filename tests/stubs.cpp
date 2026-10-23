// Test doubles for globals that the units under test expect.
// These mirror the real definitions in Calibration.cpp / CalibrationMetrics.cpp
// without pulling in the IPC client, VR runtime, or file logging.

#include <chrono>
#include "Calibration.h"
#include "CalibrationMetrics.h"

CalibrationContext CalCtx;

namespace Metrics {
	double TimeSpan = 30, CurrentTime = 0;

	TimeSeries<Eigen::Vector3d> posOffset_rawComputed;
	TimeSeries<Eigen::Vector3d> posOffset_currentCal;
	TimeSeries<Eigen::Vector3d> posOffset_lastSample;
	TimeSeries<Eigen::Vector3d> posOffset_byRelPose;

	TimeSeries<double> error_rawComputed, error_currentCal, error_byRelPose, error_currentCalRelPose;
	TimeSeries<double> axisIndependence;
	TimeSeries<double> computationTime;
	TimeSeries<double> jitterRef, jitterTarget;

	TimeSeries<bool> calibrationApplied;

	bool enableLogs = false;

	double timestamp() {
		using clock = std::chrono::steady_clock;
		static const auto start = clock::now();
		return std::chrono::duration<double>(clock::now() - start).count();
	}

	void RecordTimestamp() { CurrentTime = timestamp(); }
	void WriteLogAnnotation(const char*) { }
	void WriteLogEntry() { }
}
