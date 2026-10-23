cmake_minimum_required (VERSION 3.10)

project(SpaceCalibratorTests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Units under test: the pure-math and state helpers that don't need a live
# OpenVR session. Calibration.cpp/UserInterface.cpp/SpaceCalibrator.cpp are
# excluded — they require vr::VRSystem(), GLFW and a running SteamVR.
add_executable(SpaceCalibratorTests
	main.cpp
	stubs.cpp
	${CMAKE_SOURCE_DIR}/src/overlay/CalibrationCalc.cpp
	${CMAKE_SOURCE_DIR}/src/overlay/VRState.cpp)

target_include_directories(SpaceCalibratorTests
	PRIVATE ${CMAKE_SOURCE_DIR}/src/common
	PRIVATE ${CMAKE_SOURCE_DIR}/src/overlay
	PRIVATE ${CMAKE_SOURCE_DIR}/src/driver
	PRIVATE ${CMAKE_SOURCE_DIR}/lib
	PRIVATE ${CMAKE_SOURCE_DIR}/lib/Eigen
	PRIVATE ${CMAKE_SOURCE_DIR}/lib/openvr/headers)

# VRState.cpp references OpenVR API functions (Load() calls vr::VRSystem()).
target_link_libraries(SpaceCalibratorTests
	PRIVATE ${CMAKE_SOURCE_DIR}/lib/openvr/lib/win64/openvr_api.lib)

target_compile_definitions(SpaceCalibratorTests
	PRIVATE NOMINMAX
	PRIVATE UNICODE)

# openvr_api.dll must be resolvable at process start.
add_custom_command(
	TARGET SpaceCalibratorTests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_SOURCE_DIR}/lib/openvr/bin/win64/openvr_api.dll
	$<TARGET_FILE_DIR:SpaceCalibratorTests>/openvr_api.dll)

add_test(NAME spacecal_tests COMMAND SpaceCalibratorTests)
