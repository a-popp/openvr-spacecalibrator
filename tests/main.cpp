// Minimal dependency-free unit tests for the pure-math / IPC-adjacent units.
// Run via CTest; returns non-zero if any check fails.

#include <cmath>
#include <cstdio>
#include <random>

#include <Eigen/Dense>

#include "Calibration.h"       // pulls in Protocol.h (openvr.h before it)
#include "CalibrationCalc.h"
#include "VRState.h"
#include "IsometryTransform.h"

static int g_checks = 0, g_failures = 0;

#define CHECK(cond) do { \
		++g_checks; \
		if (!(cond)) { ++g_failures; \
			fprintf(stderr, "FAIL %s:%d: CHECK(%s)\n", __FILE__, __LINE__, #cond); } \
	} while (0)

#define CHECK_NEAR(actual, expected, eps) do { \
		++g_checks; \
		double _a = (actual), _e = (expected); \
		if (std::abs(_a - _e) > (eps)) { ++g_failures; \
			fprintf(stderr, "FAIL %s:%d: |%s - %s| = |%f - %f| > %f\n", \
				__FILE__, __LINE__, #actual, #expected, _a, _e, (double)(eps)); } \
	} while (0)

// ---------- IsoTransform ----------

static void TestIsoTransform()
{
	// Regression: the Vector3d ctor used to ignore its argument, leaving
	// translation uninitialized.
	IsoTransform t(Eigen::Vector3d(4.0, 5.0, 6.0));
	CHECK((t.translation - Eigen::Vector3d(4.0, 5.0, 6.0)).norm() < 1e-12);
	CHECK(std::abs(t.rotation.norm() - 1.0) < 1e-12);
	CHECK(t.rotation.angularDistance(Eigen::Quaterniond::Identity()) < 1e-12);

	// Composition: (a * b) * p == a * (b * p)
	IsoTransform a(
		Eigen::Quaterniond(Eigen::AngleAxisd(0.5, Eigen::Vector3d::UnitY())),
		Eigen::Vector3d(1.0, 2.0, 3.0));
	IsoTransform b(
		Eigen::Quaterniond(Eigen::AngleAxisd(-0.3, Eigen::Vector3d::UnitX())),
		Eigen::Vector3d(-1.0, 0.5, 2.0));
	Eigen::Vector3d p(0.7, -1.2, 0.4);

	Eigen::Vector3d lhs = (a * b) * p;
	Eigen::Vector3d rhs = a * (b * p);
	CHECK((lhs - rhs).norm() < 1e-12);

	// interpolateAround: endpoints must reproduce the two transforms applied
	// to the pivot point, and lerp=0/1 reproduce the transforms themselves.
	Eigen::Vector3d pivot(0.1, 1.7, -0.2);
	IsoTransform mid0 = a.interpolateAround(0.0, b, pivot);
	IsoTransform mid1 = a.interpolateAround(1.0, b, pivot);
	CHECK((mid0 * pivot - a * pivot).norm() < 1e-12);
	CHECK((mid1 * pivot - b * pivot).norm() < 1e-12);

	IsoTransform half = a.interpolateAround(0.5, b, pivot);
	Eigen::Vector3d expectedMid = (a * pivot + b * pivot) * 0.5;
	CHECK((half * pivot - expectedMid).norm() < 1e-9);
}

// ---------- DriverPoseShmem ----------

static void TestDriverPoseShmem()
{
	const char* segment = "SpaceCalibratorTestShmem";

	protocol::DriverPoseShmem writer;
	CHECK(writer.Create(segment));

	protocol::DriverPoseShmem reader;
	reader.Open(segment);
	CHECK(reader);

	vr::DriverPose_t pose{};
	pose.poseIsValid = true;
	pose.result = vr::ETrackingResult::TrackingResult_Running_OK;
	pose.vecPosition[0] = 1.5;
	pose.vecPosition[1] = -0.25;
	pose.vecPosition[2] = 0.75;
	pose.qRotation = vr::HmdQuaternion_t{ 1.0, 0.0, 0.0, 0.0 };
	pose.qWorldFromDriverRotation = vr::HmdQuaternion_t{ 1.0, 0.0, 0.0, 0.0 };

	writer.SetPose(3, pose);
	writer.SetPose(7, pose);

	// cursor/lastPose are per-instance: use a second reader for GetPose so the
	// raw drain below doesn't consume its samples.
	protocol::DriverPoseShmem reader2;
	reader2.Open(segment);
	CHECK(reader2);

	int received = 0;
	reader.ReadNewPoses([&](const protocol::DriverPoseShmem::AugmentedPose& p) {
		++received;
		CHECK(p.deviceId == 3 || p.deviceId == 7);
	});
	CHECK(received == 2);

	vr::DriverPose_t out{};
	CHECK(reader2.GetPose(3, out));
	CHECK(out.vecPosition[0] == 1.5);
	CHECK(out.poseIsValid);
	CHECK(reader2.GetPose(7, out));

	// Regression: GetPose used to fall off the end without returning on a bad index.
	CHECK(!reader2.GetPose(-1, out));
	CHECK(!reader2.GetPose(vr::k_unMaxTrackedDeviceCount, out));

	// Out-of-range writes must be dropped.
	writer.SetPose(-1, pose);
	writer.SetPose(vr::k_unMaxTrackedDeviceCount, pose);
	received = 0;
	reader.ReadNewPoses([&](const protocol::DriverPoseShmem::AugmentedPose&) { ++received; });
	CHECK(received == 0);

	reader.Close();
	reader2.Close();
	writer.Close();
}

// ---------- VRState::FindDevice ----------

static void TestFindDevice()
{
	VRState state;

	VRDevice hmd;
	hmd.id = 0;
	hmd.deviceClass = vr::TrackedDeviceClass_HMD;
	hmd.trackingSystem = "oculus";
	hmd.model = "Oculus Quest Pro";
	hmd.serial = "VRLINKHMDQUESTPRO";

	VRDevice ctrl;
	ctrl.id = 5;
	ctrl.deviceClass = vr::TrackedDeviceClass_Controller;
	ctrl.trackingSystem = "oculus";
	ctrl.model = "Quest Pro Controller";
	ctrl.serial = "CTRL123";

	VRDevice tracker;
	tracker.id = 9;
	tracker.deviceClass = vr::TrackedDeviceClass_GenericTracker;
	tracker.trackingSystem = "lighthouse";
	tracker.model = "VIVE Tracker";
	tracker.serial = "LHR-ABC";

	state.devices = { hmd, ctrl, tracker };

	// Non-HMD devices require model AND serial to match.
	CHECK(state.FindDevice("oculus", "Quest Pro Controller", "CTRL123") == 5);
	CHECK(state.FindDevice("oculus", "Quest Pro Controller", "WRONG") == -1);
	CHECK(state.FindDevice("oculus", "WRONG", "CTRL123") == -1);

	// The HMD matches on model OR serial (some drivers report inconsistent models).
	CHECK(state.FindDevice("oculus", "Oculus Quest2", "VRLINKHMDQUESTPRO") == 0);
	CHECK(state.FindDevice("oculus", "Oculus Quest Pro", "WRONG") == 0);
	CHECK(state.FindDevice("oculus", "WRONG", "WRONG") == -1);

	// Tracking system must always match.
	CHECK(state.FindDevice("lighthouse", "Quest Pro Controller", "CTRL123") == -1);
	CHECK(state.FindDevice("lighthouse", "VIVE Tracker", "LHR-ABC") == 9);
}

// ---------- CalibrationCalc ----------

static Pose MakePose(const Eigen::Matrix3d& rot, const Eigen::Vector3d& trans)
{
	Pose p;
	p.rot = rot;
	p.trans = trans;
	return p;
}

static void TestCalibrationCalc()
{
	std::mt19937 rng(42);
	std::uniform_real_distribution<double> unit(-1.0, 1.0);
	std::uniform_real_distribution<double> angle(0.5, 2.8);
	std::uniform_real_distribution<double> pos(-2.0, 2.0);

	// Ground truth calibration: pure yaw + translation (the solver only recovers yaw).
	const double yawDeg = 37.0;
	const Eigen::Vector3d expectedTrans(0.45, -0.20, 0.10);
	const Eigen::Matrix3d Crot =
		Eigen::AngleAxisd(yawDeg * EIGEN_PI / 180.0, Eigen::Vector3d::UnitY()).matrix();

	// World poses of the two devices coincide physically; the target expresses
	// them in its own world frame: R_ref = C * R_target.
	CalibrationCalc calc;
	for (int i = 0; i < 150; ++i)
	{
		Eigen::Vector3d axis(unit(rng), unit(rng), unit(rng));
		axis.normalize();
		Eigen::Matrix3d rot = Eigen::AngleAxisd(angle(rng), axis).matrix();
		Eigen::Vector3d trans(pos(rng), pos(rng), pos(rng));

		Pose ref = MakePose(rot, trans);
		Pose target = MakePose(Crot.transpose() * rot, Crot.transpose() * (trans - expectedTrans));

		calc.PushSample(Sample(ref, target, i * 0.01));
	}

	CHECK(calc.SampleCount() == 150);
	CHECK(!calc.isValid());
	CHECK(calc.ComputeOneshot(false));
	CHECK(calc.isValid());

	// eulerAngles(2,1,0) => [Z, Y, X]; Y is the yaw component.
	const Eigen::Vector3d euler = calc.EulerRotation();
	CHECK_NEAR(euler(1), yawDeg, 0.5);
	CHECK_NEAR(calc.Transformation().translation().x(), expectedTrans.x(), 1e-3);
	CHECK_NEAR(calc.Transformation().translation().y(), expectedTrans.y(), 1e-3);
	CHECK_NEAR(calc.Transformation().translation().z(), expectedTrans.z(), 1e-3);
}

static void TestCalibrationJitter()
{
	// Regression: the Welford accumulator was seeded with an uninitialized
	// Eigen::Vector3d, producing garbage/NaN jitter on identical samples.
	CalibrationCalc calc;
	Pose p = MakePose(Eigen::Matrix3d::Identity(), Eigen::Vector3d(0.5, 1.0, -0.5));
	for (int i = 0; i < 10; ++i)
		calc.PushSample(Sample(p, p, i * 0.01));

	CHECK(calc.ReferenceJitter() == 0.0);
	CHECK(calc.TargetJitter() == 0.0);
}

// ---------- CalibrationContext ----------

static void TestContextDefaults()
{
	// Regression: controllerIDs used to be uninitialized until AssignTargets ran.
	CalibrationContext ctx;
	for (size_t i = 0; i < CalibrationContext::MAX_CONTROLLERS; ++i)
		CHECK(ctx.controllerIDs[i] == -1);

	// Clearing the profile must leave the context in an uncalibrated state.
	CalCtx.validProfile = true;
	CalCtx.relativePosCalibrated = true;
	CalCtx.chaperone.valid = true;
	CalCtx.calibratedRotation = Eigen::Vector3d(1.0, 2.0, 3.0);
	CalCtx.calibratedTranslation = Eigen::Vector3d(4.0, 5.0, 6.0);
	CalCtx.Clear();
	CHECK(!CalCtx.validProfile);
	CHECK(!CalCtx.relativePosCalibrated);
	CHECK(!CalCtx.chaperone.valid);
	CHECK(CalCtx.referenceTrackingSystem.empty());
	CHECK(CalCtx.calibratedRotation.norm() == 0.0);
	CHECK(CalCtx.calibratedTranslation.norm() == 0.0);
}

int main()
{
	TestIsoTransform();
	TestDriverPoseShmem();
	TestFindDevice();
	TestCalibrationCalc();
	TestCalibrationJitter();
	TestContextDefaults();

	printf("%d checks, %d failures\n", g_checks, g_failures);
	return g_failures == 0 ? 0 : 1;
}
