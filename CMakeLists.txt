# <max> (the "...4.0" part) keeps this buildable on CMake 4.x: CMake 4.0
# hard-errors when a project's effective policy version is below 3.5 (only
# <min> below 2.8.12 was deprecated-but-tolerated before that). CMake older
# than 3.12 doesn't understand the "..." range syntax at all and just sees
# extra version-component dots, so it parses this as <min>=2.8.12 and
# ignores the rest -- see `cmake --help-command cmake_minimum_required`.
# Same fix as third_party/escargot/CMakeLists.txt, confirmed necessary here
# too on a real run once windows.yml started using a pip-installed current
# cmake instead of VS's bundled 3.31.
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12...4.0 FATAL_ERROR)
PROJECT (STARFISH)

SET (BACKEND "efl_cairo_gl" CACHE STRING "BACKEND")
SET (CUSTOM "none" CACHE STRING "CUSTOM")
SET (LTO "0" CACHE STRING "LTO")
SET (COVERAGE "0" CACHE STRING "COVERAGE")
SET (WEBGL "0" CACHE STRING "WEBGL")
SET (WEBRTC "0" CACHE STRING "WEBRTC")
SET (ENABLE_ESPLUSPLAYER "0" CACHE STRING "Use esplusplayer for MSE playback on Tizen")
SET (WORKER "0" CACHE STRING "WORKER")
SET (SHARED_WORKER "0" CACHE STRING "SHAREDWORKER")
SET (SERVICE_WORKER "0" CACHE STRING "SERVICEWORKER")
SET (IDB "0" CACHE STRING "IDB")
SET (RUNTIME_ICU "1" CACHE STRING "RUNTIME_ICU")
SET (ASAN "0" CACHE STRING "ASAN")
SET (BUILD_CAIRO "0" CACHE STRING "BUILD_CAIRO")
SET (TARGETNAME "lightweight-web-engine" CACHE STRING "TARGET NAME")
SET (OUTPUT_DIR ${CMAKE_SOURCE_DIR} CACHE STRING "OUTPUT DIR")
SET (DOCKER "0" CACHE STRING "DOCKER")
SET (TIZEN_MAJOR_VERSION "1" CACHE STRING "TIZEN_MAJOR_VERSION")
SET (TIZEN_MINOR_VERSION "0" CACHE STRING "TIZEN_MINOR_VERSION")
SET (USE_EMBEDDED_IMAGE_DECODER "0" CACHE STRING "USE_EMBEDDED_IMAGE_DECODER")
SET (ENABLE_CODECACHE "0" CACHE STRING "CODE CACHE")
SET (ENABLE_WASM "0" CACHE STRING "WEBASSEMBLY")
SET (ENABLE_DEBUGGER "0" CACHE STRING "DEBUGGER")
SET (ENABLE_TEST "0" CACHE STRING "ENABLE_TEST")
SET (ENABLE_PROFILE "0" CACHE STRING "ENABLE_PROFILE")
SET (ENABLE_TLS_ACCESS_BY_PTHREAD_KEY "1" CACHE STRING "ENABLE_TLS_ACCESS_BY_PTHREAD_KEY")
SET (TIZEN_RW_APP_DIR "./" CACHE STRING "TIZEN_RW_APP_DIR")
SET (TIZEN_DATA_DIR "./" CACHE STRING "TIZEN_DATA_ROOT_DIR")
SET (USE_FFMPEG_MEDIA_PLAYER "0" CACHE STRING "USE_FFMPEG_MEDIA_PLAYER")
OPTION (STARFISH_WINDOWS_ENABLE_MULTIMEDIA "Enable multimedia support in the Windows build" ON)
OPTION (STARFISH_WINDOWS_BUILD_SHELL "Build the minimal Win32 OpenGL shell" OFF)

# Expreimental flags for UWE
SET (ENABLE_DYNAMIC_LOADER "0" CACHE STRING "ENABLE_DYNAMIC_LOADER")
SET (ENABLE_MULTI_BACKEND "0" CACHE STRING "ENABLE_MULTI_BACKEND")

INCLUDE (ProcessorCount)
PROCESSORCOUNT (NPROCS)

# Starfish no longer has its own MODE/HOST/ARCH cache variables -- every build
# invocation (build.sh, gbs spec, CI) MUST now pass the standard CMake
# equivalents directly: -DCMAKE_BUILD_TYPE=Debug|Release,
# -DCMAKE_SYSTEM_NAME=Linux|Tizen|Windows, -DCMAKE_SYSTEM_PROCESSOR=....
# CMAKE_SYSTEM_NAME/CMAKE_SYSTEM_PROCESSOR are auto-populated by CMake itself
# for native (non-cross) builds, so only cross/Tizen/Windows configs need to
# pass them explicitly. CMAKE_BUILD_TYPE has no such default, so it is
# required here, same as this project always required MODE to be one of
# debug/release.
#
# third_party/escargot (post PR#1625 standardization) itself now selects its
# own CXXFLAGS/LDFLAGS/DEFINITIONS for Debug vs Release purely via
# $<CONFIG:Debug> generator expressions resolved from CMAKE_BUILD_TYPE -- so
# this is required for escargot's own build to behave correctly too, not just
# starfish's.
# Multi-config generators (Visual Studio, Xcode, Ninja Multi-Config) leave
# CMAKE_BUILD_TYPE empty at configure time by design -- the config is chosen
# per-build via --config, so this requirement only makes sense for
# single-config generators (Makefiles, Ninja). For those, default an unset
# CMAKE_BUILD_TYPE to Release (standard CMake practice, same default GCutil's
# own CMakeLists.txt already uses) instead of hard-failing -- but still
# reject a value that was explicitly passed and isn't one of the three this
# tree actually supports.
GET_PROPERTY (STARFISH_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
IF (NOT STARFISH_IS_MULTI_CONFIG)
    IF (NOT CMAKE_BUILD_TYPE)
        SET (CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
        SET_PROPERTY (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
        MESSAGE (STATUS "CMAKE_BUILD_TYPE not set -- defaulting to Release")
    ELSEIF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        MESSAGE (FATAL_ERROR "-DCMAKE_BUILD_TYPE=Debug, Release, or RelWithDebInfo is required (got: '${CMAKE_BUILD_TYPE}')")
    ENDIF ()
ENDIF ()

# INCLUDE CMAKE FILES

IF (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    INCLUDE (build/version.cmake)
    INCLUDE (build/windows.cmake)
ELSE ()
    INCLUDE (build/version.cmake)
    INCLUDE (build/config.cmake)
    INCLUDE (build/third_party.cmake)
    INCLUDE (build/starfish.cmake)
    INCLUDE (build/starfish_public_api.cmake)
    IF (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ${ENABLE_TEST} STREQUAL "1")
        INCLUDE (build/test/prepare_test.cmake)
    ENDIF()

    IF (SHELL)
        INCLUDE (build/starfish_shell.cmake)
    ENDIF ()

    IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
        INCLUDE (build/starfish_shell_tpk.cmake)
        SET(UWE_PACKAGE_ID "org.tizen.lightweight-web-engine")
        INCLUDE (build/starfish_uwe_tpk.cmake)
        INCLUDE (build/starfish_uwe_service.cmake)
    ENDIF()

    CONFIGURE_FILE(lightweight-web-engine.pc.in lightweight-web-engine.pc @ONLY)

    IF (${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")
        SET(WORKER "1")
        INCLUDE (build/worker.cmake)
        INCLUDE (build/worker_public_api.cmake)
        INCLUDE (build/worker_launcher.cmake)
    ENDIF()
ENDIF ()
