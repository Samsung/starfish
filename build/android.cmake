SET (OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
MESSAGE(VERBOSE "${OUTPUT_DIRECTORY}")

set (STARFISH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/starfish)
set (THIRD_PARTY_ROOT ${STARFISH_ROOT}/third_party)
set (ESCARGOT_ROOT ${STARFISH_ROOT}/third_party/escargot)

# CONFIGURE ESCARGOT VERSION
FIND_PACKAGE(Git)
IF (GIT_FOUND)
    EXECUTE_PROCESS (
            COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE ESCARGOT_BUILD_VERSION
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()
IF ((NOT DEFINED ESCARGOT_BUILD_VERSION) OR (ESCARGOT_BUILD_VERSION STREQUAL ""))
    FILE (STRINGS "${PROJECT_SOURCE_DIR}/RELEASE_VERSION" ESCARGOT_BUILD_VERSION)
ENDIF()
MESSAGE(STATUS "Escargot Build Version: ${ESCARGOT_BUILD_VERSION}")
CONFIGURE_FILE (${ESCARGOT_ROOT}/src/EscargotInfo.h.in ${OUTPUT_DIRECTORY}/escargot_generated/EscargotInfo.h @ONLY)

# JS BINDING
# Generate binding code first
#
# Two separate EXECUTE_PROCESS calls, not one with two COMMANDs: multiple
# COMMANDs in a single execute_process() are chained via a pipe (stdout of
# the first feeds stdin of the second), not run sequentially -- harmless
# here since make_directory prints nothing, but the real reason for the
# split is RESULT_VARIABLE below. Without it, a codegen failure (e.g. a
# missing python3 module) left the generated dir empty and CMake carried
# on regardless, only surfacing as a confusing "binding/generated/
# Interfaces.h file not found" compile error much later.
EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/
)
EXECUTE_PROCESS(
        COMMAND python3 ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/
        RESULT_VARIABLE STARFISH_BINDING_GEN_RESULT
)
IF (NOT STARFISH_BINDING_GEN_RESULT EQUAL 0)
    MESSAGE(FATAL_ERROR "starfish_code_generator.py failed (exit code ${STARFISH_BINDING_GEN_RESULT}) -- see its output above for the actual error.")
ENDIF()

# (Removed: an early debug/release IF(CMAKE_BUILD_TYPE STREQUAL ...) block
# used to set LWE_CFLAGS/LWE_CXXFLAGS here. Dead regardless of whether
# CMAKE_BUILD_TYPE is set by the caller's build.gradle: LWE_CXXFLAGS gets
# fully reassigned -- not appended to -- further down, and LWE_CFLAGS is
# never consumed by any target_compile_options/definitions call in this file.)

#######################################################
# ESCARGOT (+ GCutil, libbf, libsimdutf, runtime_icu_binder)
#######################################################
# Previously this file hand-reimplemented escargot's build: it globbed
# escargot's own src/**/*.cpp directly into the `lwe` SHARED library's source
# list, ran escargot's codegen scripts (UnicodeIdentifierTables.cpp,
# YarrCanonicalizeUnicode.cpp, UnicodePatternTables.h, YarrCanonicalizeUCS2.cpp)
# by hand, and hand-rolled a `gcutil` STATIC target with a manually-maintained
# ~20-define GCUTIL_CFLAGS_INTERNAL list -- all duplicating (and silently
# drifting from) what third_party/escargot's own CMakeLists.txt already does,
# the same way linux/tizen (third_party.cmake) and windows (windows.cmake)
# consume it via ADD_SUBDIRECTORY. escargot's CMakeLists.txt already has
# first-class ESCARGOT_HOST=android support (TLS-by-pthread-key selection,
# -DANDROID=1/-DESCARGOT_ANDROID=1, -mstackrealign, per-ESCARGOT_ARCH 32/64-bit
# flags -- see third_party/escargot/CMakeLists.txt and build/target.cmake), so
# delegate to it the same way every other platform does.
#
# ESCARGOT_THREADING is explicitly OFF here (unlike third_party.cmake/
# windows.cmake, which turn it ON) to preserve this file's previous behavior:
# the hand-rolled gcutil build never defined GC_THREAD_ISOLATE/_REENTRANT or
# linked libatomic, i.e. Android has never shipped with Atomics/
# SharedArrayBuffer. Flip this on deliberately (and add `atomic` to
# LWE_LINK_LIBRARIES, which escargot's target.cmake requests via
# ESCARGOT_LIBRARIES when ESCARGOT_THREADING is ON) if that's ever wanted.
IF (${ANDROID_SYSROOT_ABI} STREQUAL "arm64")
    SET (ESCARGOT_ARCH aarch64)
ELSEIF (${ANDROID_SYSROOT_ABI} STREQUAL "arm")
    SET (ESCARGOT_ARCH arm)
ELSEIF (${ANDROID_SYSROOT_ABI} STREQUAL "x86_64")
    SET (ESCARGOT_ARCH x64)
ELSEIF (${ANDROID_SYSROOT_ABI} STREQUAL "x86")
    SET (ESCARGOT_ARCH x86)
ENDIF()
SET (ESCARGOT_HOST android)
STRING (TOLOWER "${CMAKE_BUILD_TYPE}" ESCARGOT_MODE)
SET (ESCARGOT_BUILD_SHARED_LIBS OFF)
SET (ESCARGOT_ENABLE_SHELL OFF)
SET (ESCARGOT_THREADING OFF)
SET (ESCARGOT_USE_CUSTOM_LOGGING ON)
IF (ENABLE_RUNTIME_ICU_BINDER)
    SET (ESCARGOT_LIBICU_SUPPORT ON)
    SET (ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN ON)
ENDIF()

# Unlike linux/tizen (third_party.cmake) and windows (windows.cmake), this
# file is INCLUDE()'d from lwe_android_rel's own CMakeLists.txt (a different
# repo, where starfish is checked out as a subdirectory rather than being the
# project root) -- so CMAKE_CURRENT_SOURCE_DIR there is NOT the starfish repo
# root, and a bare relative "third_party/escargot" resolves to the wrong
# path. Use the absolute ESCARGOT_ROOT instead; CMake then requires an
# explicit binary dir since the source dir isn't under CMAKE_CURRENT_SOURCE_DIR.
ADD_SUBDIRECTORY (${ESCARGOT_ROOT} ${OUTPUT_DIRECTORY}/escargot)

#######################################################
# SOURCE FILES
#######################################################

file(GLOB STARFISH_SRC "${STARFISH_ROOT}/src/*.cpp" )
file(GLOB_RECURSE STARFISH_GEN_SRC "${OUTPUT_DIRECTORY}/starfish_generated/binding/**/*.cpp" )
file(GLOB_RECURSE STARFISH_SUB_SRC "${STARFISH_ROOT}/src/**/*.cpp" )
FILE (GLOB_RECURSE STARFISH_SHELL_SRC ${STARFISH_ROOT}/src/shell/*.cpp)

LIST (REMOVE_ITEM STARFISH_SUB_SRC
        ${STARFISH_SHELL_SRC}
        )

#######################################################
# INCLUDE DIRS
#######################################################

set(LWE_INCLUDE_DIRS
        ${STARFISH_ROOT} ${STARFISH_ROOT}/inc ${STARFISH_ROOT}/src
        ${OUTPUT_DIRECTORY}/starfish_generated/
        ${OUTPUT_DIRECTORY}/escargot_generated/
        ${OUTPUT_DIRECTORY}/escargot_generated/yarr/
        ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil/include
        ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil/include/gc
        ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil
        ${STARFISH_ROOT}/third_party/escargot/src/
        ${STARFISH_ROOT}/third_party/escargot/src/api
        ${STARFISH_ROOT}/third_party/escargot/third_party/
        ${STARFISH_ROOT}/third_party/escargot/third_party/checked_arithmetic
        ${STARFISH_ROOT}/third_party/escargot/third_party/double_conversion
        ${STARFISH_ROOT}/third_party/escargot/third_party/rapidjson/include
        ${STARFISH_ROOT}/third_party/escargot/third_party/yarr
        ${STARFISH_ROOT}/third_party/escargot/third_party/runtime_icu_binder
        ${STARFISH_ROOT}/third_party/escargot/third_party/libbf
        ${STARFISH_ROOT}/third_party/escargot/third_party/simdutf
        ${STARFISH_ROOT}/third_party/escargot/third_party/xsum
        ${THIRD_PARTY_ROOT}/clipper/cpp
        ${THIRD_PARTY_ROOT}/rapidxml
        ${THIRD_PARTY_ROOT}/earcut.hpp/include/mapbox
        ${THIRD_PARTY_ROOT}/third_party/clipper/cpp
        ${THIRD_PARTY_ROOT}/skia_matrix/
        ${THIRD_PARTY_ROOT}/skia_matrix/include/core/
        ${THIRD_PARTY_ROOT}/skia_matrix/include/private/
        ${THIRD_PARTY_ROOT}/MP4Parse/source/include
        ${THIRD_PARTY_ROOT}/webm/
        ${THIRD_PARTY_ROOT}/robin_map/include
        )

#######################################################
# DEFINITION
#######################################################

set(LWE_DEFINITIONS
        -DANDROID
        -DSTARFISH_BACKEND_STR="uv_cairo_gl"
        -DSTARFISH_VERSION_STR="Starfish for android"
        -DSTARFISH_ANDROID
        -DSTARFISH_CANVAS_SURFACE_TILE_SIZE=128
        -DSTARFISH_ENABLE_ANIMATION
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX
        -DSTARFISH_ENABLE_OBSOLETE_SPEC
        -DSTARFISH_ENABLE_WEBSOCKET
        -DESCARGOT
        -DENABLE_ICU
        -DENABLE_INTL
        -DENABLE_RELOADABLE_STRING
        -DSkDebugf=printf
        -DSK_RELEASE
        )

IF (${ANDROID_SYSROOT_ABI} STREQUAL "arm64")
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
            -DESCARGOT_64=1
            -DESCARGOT_USE_32BIT_IN_64BIT=1
            )
ELSEIF(${ANDROID_SYSROOT_ABI} STREQUAL "arm")
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
            -DESCARGOT_32=1)
ELSEIF(${ANDROID_SYSROOT_ABI} STREQUAL "x86_64")
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
            -DESCARGOT_64=1
            -DESCARGOT_USE_32BIT_IN_64BIT=1
            )
ELSEIF(${ANDROID_SYSROOT_ABI} STREQUAL "x86")
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
            -DESCARGOT_32=1)
ENDIF()

IF (ENABLE_RUNTIME_ICU_BINDER)
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
            -DSTARFISH_ENABLE_RUNTIME_ICU_BINDER=1
            -DENABLE_RUNTIME_ICU_BINDER=1
            )
ENDIF()


#######################################################
# CXXFLAGS & LDFLAGS
#######################################################

set(LWE_CXXFLAGS_NDK ${NDK_CXXFLAGS})
SEPARATE_ARGUMENTS(LWE_CXXFLAGS_NDK)
set(LWE_CXXFLAGS
        ${LWE_CXXFLAGS_NDK}
        -Wall
        -Wno-format-nonliteral
        -Wno-invalid-offsetof
        -Wno-unused-parameter
        -Wno-unused-result
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-deprecated-declarations
        -Wno-type-limits
        -Wno-parentheses-equality
        -Wno-unused-parameter
        -Wno-dynamic-class-memaccess
        -Wno-deprecated-register
        -Wno-expansion-to-defined
        -Wno-return-type
        -Wno-type-limits
        -Wno-unused-result
        -Wno-narrowing
        -Wno-inconsistent-missing-override
        -Wno-mismatched-tags
        -Wno-deprecated-register
        -Wno-expansion-to-defined
        -Wno-return-type
        -Wno-overloaded-virtual
        -Wno-unused-private-field
        -fno-fast-math
        -fno-unsafe-math-optimizations
        -fno-rtti
        -fexceptions
        -fvisibility=hidden
        -fno-omit-frame-pointer
        -fstack-protector
        -fno-math-errno
        -fdata-sections
        -ffunction-sections
        -fno-fast-math
        -fdenormal-fp-math=ieee
        )

# Generator expressions instead of a configure-time IF(CMAKE_BUILD_TYPE ...):
# this is a single-config-per-variant CMake invocation (Gradle reconfigures
# separately per Debug/Release build type), so $<CONFIG:Debug> resolves
# correctly whether or not CMAKE_BUILD_TYPE happens to be set by the time
# this line runs -- a plain IF() would silently no-op if it weren't yet.
# (The matching LWE_CFLAGS append was dropped: LWE_CFLAGS is never consumed
# by any target_compile_options/definitions call in this file.)
SET(LWE_CXXFLAGS ${LWE_CXXFLAGS}
    $<$<CONFIG:Debug>:-DGC_DEBUG>
    $<$<CONFIG:Debug>:-g3>
    $<$<NOT:$<CONFIG:Debug>>:-DNDEBUG>
    $<$<NOT:$<CONFIG:Debug>>:-Oz>
    $<$<NOT:$<CONFIG:Debug>>:-g3>
)

set(LWE_LDFLAGS ${NDK_LDFLAGS_SHARED})

#######################################################
# LIBRARIES
#######################################################
find_library(log-lib log)
find_library(z-lib z)
find_library(android-lib android)

#######################################################
# THIRD PARTY
#######################################################
SET (THIRD_PARTY_CXXFLAGS_COMMON -std=c++11 -fvisibility=default -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fno-omit-frame-pointer -fstack-protector -fPIC)

SET (THIRD_PARTY_CXXFLAGS ${LWE_CXXFLAGS} ${THIRD_PARTY_CXXFLAGS_COMMON})
SET (THIRD_PARTY_DEFINITIONS ${LWE_DEFINITIONS})


#######################################################
# SKIA_MATRIX
#######################################################
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_CORE ${THIRD_PARTY_ROOT}/skia_matrix/src/core/*.cpp)
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_PORTS ${THIRD_PARTY_ROOT}/skia_matrix/src/ports/*.cpp)
ADD_LIBRARY (skia_matrix SHARED ${SKIA_MATRIX_SRC_CORE} ${SKIA_MATRIX_SRC_PORTS})
TARGET_INCLUDE_DIRECTORIES (skia_matrix PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix ${THIRD_PARTY_ROOT}/skia_matrix/include/core ${THIRD_PARTY_ROOT}/skia_matrix/include/private)
TARGET_COMPILE_DEFINITIONS (skia_matrix PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (skia_matrix PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES(skia_matrix PROPERTIES LINK_FLAGS ${NDK_LDFLAGS_SHARED})


#######################################################
# CLIPPER
#######################################################
FILE (GLOB CLIPPER_SRC ${THIRD_PARTY_ROOT}/clipper/cpp/*.cpp)
ADD_LIBRARY (clipper SHARED ${CLIPPER_SRC})
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (clipper PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES(clipper PROPERTIES LINK_FLAGS ${NDK_LDFLAGS_SHARED})


#######################################################
# MP4PARSE
#######################################################
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES(mp4parse PROPERTIES LINK_FLAGS ${NDK_LDFLAGS_SHARED})


#######################################################
# WEBM
#######################################################
ADD_LIBRARY (webm SHARED
        ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
        ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
        )
TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)
TARGET_COMPILE_DEFINITIONS (webm PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (webm PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES(webm PROPERTIES LINK_FLAGS ${NDK_LDFLAGS_SHARED})


#######################################################
# LIBWEBSOCKETS
#######################################################
SET(LIBWEBSOCKETS_SOURCE_DIR ${THIRD_PARTY_ROOT}/libwebsockets/)
SET(LIBWEBSOCKETS_BUILD_DIR ${OUTPUT_DIRECTORY}/libwebsockets/)
SET(LIBWEBSOCKETS_BUILD_OUTDIR ${OUTPUT_DIRECTORY}/libwebsockets/build)
SET(LIBWEBSOCKETS_LOCAL_TARGET ${LIBWEBSOCKETS_BUILD_OUTDIR}/lib/libwebsockets_lwe.so)
SET(LIBWEBSOCKETS_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebsockets_lwe.so)
SET(OPENSSL_LIB_CUSTOM "-DLWS_OPENSSL_LIBRARIES=\"${OPENSSL_BUILD_PATH}/lib/libssl.so;${OPENSSL_BUILD_PATH}/lib/libcrypto.so\"")

ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
        COMMENT "COPY LIBWEBSOCKETS SOURCE"
        COMMAND cp -r ${LIBWEBSOCKETS_SOURCE_DIR} ${OUTPUT_DIRECTORY}
        COMMAND sed -i "s/hidden/default/" ${LIBWEBSOCKETS_BUILD_DIR}/include/libwebsockets.h
        COMMAND touch ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
        )

SET(LIBWEBSOCKETS_BUILD_OPTION -DSTARFISH_CUSTOM=1 -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF
        -DLWS_HAVE_VISIBILITY:BOOL=ON
        -DOPENSSL_ROOT_DIR=${OPENSSL_BUILD_PATH}
        -DLWS_OPENSSL_INCLUDE_DIRS=${OPENSSL_BUILD_PATH}/include
        -DLWS_WITH_HTTP_STREAM_COMPRESSION=ON
        -DLWS_WITH_ZLIB=ON
        -DLWS_ZLIB_LIBRARIES=${CONFIGURE_SYSROOT_LIB}/libz.so
        -DLWS_ZLIB_INCLUDE_DIRS=${CMAKE_SYSROOT}/include/
        )

SET(LIBWEBSOCKETS_CFLAGS "${NDK_CFLAGS} -Wno-unknown-warning-option")
SET(LIBWEBSOCKETS_CXXFLAGS "${NDK_CXXFLAGS} -Wno-unknown-warning-option")

ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
        DEPENDS openssl ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
        WORKING_DIRECTORY ${LIBWEBSOCKETS_BUILD_DIR}
        COMMENT "BUILD LIBWEBSOCKETS"
        COMMAND export PATH=${ANDROID_TOOLCHAIN_ROOT}/bin:$$PATH
        COMMAND export AR=${NDK_AR_PATH}
        COMMAND export AS=${NDK_AS_PATH}
        COMMAND export CC=${NDK_CC_PATH}
        COMMAND export CXX=${NDK_CXX_PATH}
        COMMAND export LD=${NDK_LD_PATH}
        COMMAND export RANLIB=${NDK_RANLIB_PATH}
        COMMAND export STRIP=${NDK_STRIP_PATH}
        COMMAND export CFLAGS=${LIBWEBSOCKETS_CFLAGS}
        COMMAND export CXXFLAGS=${LIBWEBSOCKETS_CXXFLAGS}
        COMMAND export LDFLAGS=${NDK_LDFLAGS_SHARED}
        COMMAND export ASFLAGS=${NDK_ASFLAGS}
        COMMAND export CCASFLAGS=${NDK_ASFLAGS}
        COMMAND export PKG_CONFIG_PATH=${NDK_PKG_CONFIG_PATH}
        COMMAND export PKG_CONFIG_LIBDIR=${NDK_PKG_CONFIG_LIBDIR}
        COMMAND export PKG_CONFIG_SYSROOT_DIR=${NDK_PKG_CONFIG_SYSROOT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_OUTDIR}
        COMMAND ${CMAKE_COMMAND} -S . -B${LIBWEBSOCKETS_BUILD_OUTDIR} -G Ninja ${LIBWEBSOCKETS_BUILD_OPTION}
            "${OPENSSL_LIB_CUSTOM}"
            -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_PLATFORM=android-${ANDROID_PLATFORM_LEVEL}
            -DANDROID_TOOLCHAIN=clang
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_ANDROID_NDK}/build/cmake/android.toolchain.cmake
            -DCMAKE_C_FLAGS=${LIBWEBSOCKETS_CFLAGS}
            -DCMAKE_CXX_FLAGS=${LIBWEBSOCKETS_CXXFLAGS}
        COMMAND ${CMAKE_COMMAND} --build ${LIBWEBSOCKETS_BUILD_OUTDIR}
        COMMAND cp -r ${LIBWEBSOCKETS_BUILD_DIR}/include ${LIBWEBSOCKETS_BUILD_OUTDIR}
        )

ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_TARGET}
        DEPENDS ${LIBWEBSOCKETS_LOCAL_TARGET}
        COMMENT "COPY LIBWEBSOCKETS"
        COMMAND cp ${LIBWEBSOCKETS_LOCAL_TARGET} ${LIBWEBSOCKETS_TARGET}
        )

ADD_CUSTOM_TARGET (libwebsockets
        DEPENDS ${LIBWEBSOCKETS_TARGET}
        COMMENT "LIBWEBSOCKETS TARGET"
        )

SET(LWE_INCLUDE_DIRS ${LWE_INCLUDE_DIRS} ${LIBWEBSOCKETS_BUILD_OUTDIR}/include)

#######################################################
# BUILD TARGET
#######################################################
# (libbf/libsimdutf/runtime-icu-binder-static/gc-lib are no longer built or
# globbed here -- they're built by third_party/escargot's own CMakeLists.txt
# via ADD_SUBDIRECTORY above, and PUBLIC-linked into the `escargot` target,
# so linking `escargot` below pulls them in transitively.)

add_library(lwe
        SHARED
        ${STARFISH_SRC}
        ${STARFISH_GEN_SRC}
        ${STARFISH_SUB_SRC}
        )

target_compile_definitions(lwe PRIVATE ${LWE_DEFINITIONS})
target_compile_options(lwe PRIVATE ${LWE_CXXFLAGS})
target_include_directories(lwe PRIVATE ${LWE_INCLUDE_DIRS})
target_link_libraries(lwe ${LWE_LDFLAGS})
set_target_properties(lwe PROPERTIES LINKER_LANGUAGE CXX)

ADD_CUSTOM_COMMAND(TARGET lwe POST_BUILD
        COMMAND cp -P -u ${NGHTTP2_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${OPENSSL_BUILD_TEMP_PATH}/lib*so* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${CURL_BUILD_PATH}/lib/lib*so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${PNG_BUILD_PATH}/lib/libpng16.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${JPEG_BUILD_PATH}/libjpeg.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${FONTCONFIG_BUILD_TEMP_PATH}/src/.libs/libfontconfig.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${CAIRO_BUILD_PATH}/lib/libcairo*.so* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        COMMAND cp -P -u ${UV_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
        )

IF (NOT ENABLE_RUNTIME_ICU_BINDER)
    ADD_CUSTOM_COMMAND(TARGET lwe POST_BUILD
            COMMAND cp -P -u ${ICU_BUILD_PATH}/lib/lib*so* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
            )
ENDIF()

SET(LWE_LINK_LIBRARIES
        escargot
        skia_matrix
        clipper
        mp4parse
        webm
        cpufeatures
        openssl
        curl
        png
        jpeg
        gif
        ft2
        harfbuzz
        expat
        fontconfig
        ${LIBWEBSOCKETS_TARGET}
        cairo
        tuv
        GLESv3
        EGL
)

IF (NOT ENABLE_RUNTIME_ICU_BINDER)
    SET(LWE_LINK_LIBRARIES
        ${LWE_LINK_LIBRARIES}
        icu
    )
ENDIF()

target_link_libraries( # Specifies the target library.
        lwe
        ${log-lib}
        ${android-lib}
        ${LWE_LINK_LIBRARIES}
        )

ADD_DEPENDENCIES (lwe libwebsockets)
