cmake_minimum_required(VERSION 2.8.12...4.0 FATAL_ERROR)
include(CheckLibraryExists)

#######################################################
# PATH
#######################################################

SET (STARFISH_ROOT ${CMAKE_SOURCE_DIR})
SET (THIRD_PARTY_ROOT ${STARFISH_ROOT}/third_party)
SET (ESCARGOT_ROOT ${THIRD_PARTY_ROOT}/escargot)
SET (ESCARGOT_THIRD_PARTY_ROOT ${ESCARGOT_ROOT}/third_party)
SET (GCUTIL_ROOT ${ESCARGOT_THIRD_PARTY_ROOT}/GCutil)
SET (TOOL_ROOT ${STARFISH_ROOT}/tool)

STRING(TOLOWER "${CMAKE_SYSTEM_NAME}" HOST_LOWER)
STRING(TOLOWER "${CMAKE_BUILD_TYPE}" MODE_LOWER)
IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET(ARCH_LOWER "x64")
ELSE()
    SET(ARCH_LOWER "${CMAKE_SYSTEM_PROCESSOR}")
ENDIF()

#######################################################
# OUTPUT PATH
#######################################################

IF (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    SET (OUTPUT_DIRECTORY ${OUTPUT_DIR}/out/${MODE_LOWER})
ELSE()
    SET (OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
ENDIF()

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/lib)
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/lib)

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen") # this needs for gbs build
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
ELSE()
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/bin)
ENDIF()



#######################################################
# DEFINITION
#######################################################

# DEFINITION Description
# STARFISH_ENABLE_MULTIMEDIA : enable multimedia element (video, audio, track) features
# STARFISH_ENABLE_INSPECTOR : enable inspector which is used for message sender in separate thread
# STARFISH_ENABLE_CDP : enable Chrome DevTools Protocol server (Target/Page/Runtime/DOM/Log)
# STARFISH_ENABLE_TTS : enable TTS (Text-To-Speech)
# STARFISH_ENABLE_A11Y_TOUCH_EXPLORATION : enable touch-exploration accessibility (tap=speak aria-label, double-tap=activate, swipe=next/prev). Non-TV profiles only. Controlled by cmake option ENABLE_A11Y_TOUCH.
# STARFISH_ENABLE_HTTPCACHE : enable HTTPCache feature which caches resources downloaded through HTML
# STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING : enable multi threaded image decoding
# STARFISH_TIZEN : enable several TIZEN specific features such as media player, backend graphic library
# TIZEN_DEVICE_API : enable TIZEN device API Loader for escargot. define JS Object extension which has supported TIZEN properties.
# SIZE_MAX=0xffffffff : define maximum size of 32bit unsigned value
# STARFISH_IGNORE_SSL_VERIFYPEER : ignore SSL connection verification only for Android
# STARFISH_ENABLE_TEST : enable features only necessary for TC runs
# STARFISH_MEDIAPLAYER_DEBUG : enable debugging and messaging for mediaplayer
# STARFISH_TIZEN_TV : enable features only necessary for TIZEN based TV targets
# STARFISH_TIZEN_PROD_TV : enable features only necessary for TIZEN based TV targets for product
# STARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED : enable TIZEN specific GEOLOCATION feature
# STARFISH_TIZEN_USERAPP_SDK_API_ONLY : enable try to use public tizen api(in userapp sdk) only
# STARFISH_ENABLE_AVPLAY : enable AVPLAY only necessary for TIZEN based TV targets
# STARFISH_ENABLE_TRANSPARENT_WINDOW : enable transparent window (transparent background) currently necessary for TIZEN based TV targets
# STARFISH_ENABLE_BODY_FOCUS_RING : draw focus ring when focus event occurred
# STARFISH_ENABLE_VIRTUAL_CURSOR : enable painting of virtual cursor
# STARFISH_TIZEN_WEARABLE_WIDGET : enable features only necessary for TIZEN wearable targets
# STARFISH_DISABLE_OVERFLOW_SCROLL : disable scroll event for wearable targets
# STARFISH_ENABLE_CANVAS : enable HTMLCanvasElement
# STARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX: enable CSS -webkit-transform-* support
# STARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX: enable CSS -webkit-flex-* support
# STARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX: enable CSS -webkit-transition-* support
# STARFISH_ENABLE_OBSOLETE_SPEC : enable obsolete spec
# STARFISH_ENABLE_BATTERY_STATUS : enable battery status api
# STARFISH_ENABLE_WEBRTC: enable WebRTC
# STARFISH_ENABLE_WEBSOCKET: enable WebSocket spec
# STARFISH_ENABLE_WASM : enable WebAssembly
# STARFISH_ENABLE_IDB : enable IndexedDB
# _GLIBCXX_DEBUG : GNU compiler compiles user code using the debug mode


SET (LWE_DEFINES_DEFAULT -DSTARFISH_VERSION_STR="${LWE_VERSION}")
FILE (WRITE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/VERSION "${LWE_VERSION}")

# Touch-exploration accessibility. Off by default on Tizen; opt-in via
# -DENABLE_A11Y_TOUCH=1. Force-disabled on TV profiles (see below).
IF (NOT DEFINED ENABLE_A11Y_TOUCH)
    SET (ENABLE_A11Y_TOUCH "0")
ENDIF()

SET (USE_CUSTOM_WEBP "0")

IF (${DOCKER} STREQUAL "1")
    SET (LWE_DEFINES_DEFAULT
        ${LWE_DEFINES_DEFAULT}
        -DSTARFISH_DOCKER
    )
ENDIF()

IF (${ENABLE_PROFILE} STREQUAL "1")
    SET (LWE_DEFINES_DEFAULT
        ${LWE_DEFINES_DEFAULT}
        -DSTARFISH_ENABLE_PROFILE
    )
ENDIF()

# WebSocket is backed by the libwebsockets sub-build, which only these hosts and
# profiles ship. Everything that depends on it - the STARFISH_ENABLE_WEBSOCKET
# define, the sub-build in third_party.cmake, its target dependency in
# starfish.cmake and the link against websockets_lwe - keys off this single flag,
# so a profile can never end up compiling WebSocket without its library.
IF (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ${CUSTOM} STREQUAL "prod_tv" OR ${CUSTOM} STREQUAL "unified_tv" OR ${CUSTOM} STREQUAL "unified_mobile" OR ${CUSTOM} STREQUAL "unified_wearable" OR ${CUSTOM} STREQUAL "flutter")
    SET (USE_LIBWEBSOCKETS "1")
ELSE()
    SET (USE_LIBWEBSOCKETS "0")
ENDIF()

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET (LWE_DEFINES_ARCH
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_INSPECTOR
        -DSTARFISH_ENABLE_TTS
        -DSTARFISH_ENABLE_A11Y_TOUCH_EXPLORATION
        -DSTARFISH_ENABLE_HTTPCACHE
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_OBSOLETE_SPEC
        -DSTARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_LINE_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX
        -DSTARFISH_ENABLE_ANIMATION
        -DSTARFISH_ENABLE_WEBAUDIO
    )
ELSEIF (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm|x86)$")
    SET (LWE_DEFINES_ARCH
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_OBSOLETE_SPEC
        -DSTARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_LINE_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX
        -DSTARFISH_ENABLE_ANIMATION
    )
ENDIF()

IF (${USE_LIBWEBSOCKETS} STREQUAL "1")
    SET (LWE_DEFINES_ARCH
        ${LWE_DEFINES_ARCH}
        -DSTARFISH_ENABLE_WEBSOCKET
    )
ENDIF()

# STARFISH_ENABLE_CDP : enable Chrome DevTools Protocol server. Off by default;
# pass -DSTARFISH_ENABLE_CDP=1 at configure time to enable.
IF (STARFISH_ENABLE_CDP)
    SET (LWE_DEFINES_DEFAULT
        ${LWE_DEFINES_DEFAULT}
        -DSTARFISH_ENABLE_CDP
    )
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(LWE_DEFINES_HOST
        -DSTARFISH_LINUX
    )
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET(LWE_DEFINES_HOST
        -DSTARFISH_TIZEN
        -DSTARFISH_TIZEN_OBS
        -DTIZEN_DEVICE_API
        -DSIZE_MAX=0xffffffff
        -DSTARFISH_ENABLE_ANIMATION
    )
    IF (${ENABLE_TEST} STREQUAL "1")
        SET(LWE_DEFINES_HOST
            ${LWE_DEFINES_HOST}
            -DSTARFISH_ENABLE_TEST
        )
    ENDIF()
ENDIF()

IF (${CUSTOM} STREQUAL "unified_mobile")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
    )
    # esplusplayer MSE backend is only supported on Tizen 10 or higher.
    IF ((${TIZEN_MAJOR_VERSION} GREATER 10) OR (${TIZEN_MAJOR_VERSION} EQUAL 10))
        SET (ENABLE_ESPLUSPLAYER "1")
    ENDIF()
ELSEIF (${CUSTOM} STREQUAL "unified_tv")
    SET (LWE_DEFINES_CUSTOM
        #-DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
        -DSTARFISH_TIZEN_TV
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        #-DSTARFISH_ENABLE_AVPLAY
        #-DSTARFISH_ENABLE_TRANSPARENT_WINDOW
        #-DSTARFISH_ENABLE_TTS
        #-DSTARFISH_ENABLE_BODY_FOCUS_RING
        #-DSTARFISH_ENABLE_VIRTUAL_CURSOR
        #-DUSE_PRODUCT_FEATURE
        #-DSTARFISH_ENABLE_WEBAUDIO
        -DSTARFISH_TIZEN_USERAPP_SDK_API_ONLY
    )

ELSEIF (${CUSTOM} STREQUAL "prod_tv")
    SET (LWE_DEFINES_CUSTOM
        #-DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_TIZEN_TV
        -DSTARFISH_TIZEN_PROD_TV
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        #-DSTARFISH_ENABLE_AVPLAY
        #-DSTARFISH_ENABLE_TRANSPARENT_WINDOW
        -DSTARFISH_ENABLE_TTS
        #-DSTARFISH_ENABLE_BODY_FOCUS_RING
        #-DSTARFISH_ENABLE_VIRTUAL_CURSOR
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_OBSOLETE_SPEC
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
        -DSTARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_LINE_PREFIX
        -DUSE_PRODUCT_FEATURE
        #-DSTARFISH_ENABLE_WEBAUDIO
        -DSTARFISH_TIZEN_USERAPP_SDK_API_ONLY
    )
ELSEIF (${CUSTOM} STREQUAL "unified_wearable")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_TIZEN_WEARABLE_WIDGET
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        -DSTARFISH_DISABLE_OVERFLOW_SCROLL
        #-DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_OBSOLETE_SPEC
        -DSTARFISH_ENABLE_BATTERY_STATUS
    )
ELSEIF (${CUSTOM} STREQUAL "headless")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_TIZEN_HEADLESS
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        #-DSTARFISH_DISABLE_OVERFLOW_SCROLL
        -DSTARFISH_ENABLE_MULTIMEDIA
    )

ENDIF()

# Touch-exploration accessibility for non-TV Tizen profiles. TV profiles keep
# their existing behavior untouched (macro stays undefined there).
IF (${ENABLE_A11Y_TOUCH} STREQUAL "1" AND CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    IF (${CUSTOM} STREQUAL "unified_tv" OR ${CUSTOM} STREQUAL "prod_tv")
        MESSAGE (WARNING "ENABLE_A11Y_TOUCH ignored on TV profiles (${CUSTOM})")
    ELSE()
        SET (LWE_DEFINES_CUSTOM
            ${LWE_DEFINES_CUSTOM}
            -DSTARFISH_ENABLE_A11Y_TOUCH_EXPLORATION
            -DSTARFISH_ENABLE_TTS
        )
    ENDIF()
ENDIF()

# ELSE (not an exact-string ELSEIF allowlist) so every non-Debug config
# (Release, RelWithDebInfo, MinSizeRel, ...) gets -DNDEBUG instead of the old
# ELSEIF's "Release"/"RelWithDebInfo" verbatim match silently dropping it (or,
# for anything else, hitting the FATAL_ERROR below even on a value the
# top-level CMakeLists.txt already accepts).
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET (LWE_DEFINES_MODE
        -DGC_DEBUG # bdwgc
        -D_GLIBCXX_DEBUG
        -DSTARFISH_ENABLE_TEST
    )
ELSE()
    SET (LWE_DEFINES_MODE -DNDEBUG)
    IF (${ENABLE_TEST} STREQUAL "1")
        SET(LWE_DEFINES_MODE ${LWE_DEFINES_MODE} -DSTARFISH_ENABLE_TEST)
    ENDIF()
ENDIF()

IF (${BACKEND} STREQUAL "flutter")
    IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
        SET (USE_CUSTOM_WEBP "1")
        SET (LWE_DEFINES_BACKEND
            -DSTARFISH_FLUTTER
            # -DSTARFISH_ENABLE_MULTIMEDIA
            -DSTARFISH_ENABLE_CANVAS
            -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
            -DUSE_CUSTOM_WEBP
        )
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "uv_cairo_gl")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_UV_CAIRO_GL)
ELSEIF (${BACKEND} STREQUAL "glib_cairo_gl")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_GLIB_CAIRO_GL)
ELSEIF (${BACKEND} STREQUAL "glib_headless")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_GLIB_HEADLESS -DSTARFISH_HEADLESS)
ELSE ()
    MESSAGE (FATAL_ERROR "BACKEND is NOT SET.(ex. glib_cairo_gl, glib_headless, uv_cairo_gl, flutter)")
ENDIF()

SET (LWE_DEFINES_BACKEND ${LWE_DEFINES_BACKEND}
    -DSTARFISH_BACKEND_STR="${BACKEND}"
)

#######################################################
# SHELL DEFINES
#######################################################

INCLUDE(${STARFISH_ROOT}/build/starfish_shell_defines.cmake)
SET_STARFISH_SHELL_DEFINES()

# Tmp disable WebRTC on Linux until openssl1.1 is installed on all dev machines
IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # SET (WEBRTC "1")
ELSEIF ((${CUSTOM} STREQUAL "unified_tv" OR ((${CUSTOM} STREQUAL "prod_tv") AND (${ENABLE_TEST} STREQUAL "1"))) AND ((${TIZEN_MAJOR_VERSION} GREATER 6) OR (${TIZEN_MAJOR_VERSION} EQUAL 6)))
ENDIF()

IF (${WEBRTC} STREQUAL "1")
    # WebRTC signaling goes through WebSocket, so it cannot be built on a profile
    # that has no libwebsockets. Report that here instead of letting the WebSocket
    # sources fail later on the missing libwebsockets.h.
    IF (NOT ${USE_LIBWEBSOCKETS} STREQUAL "1")
        MESSAGE (FATAL_ERROR "WEBRTC=1 requires WebSocket, which is not available for CUSTOM=${CUSTOM} on CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
    ENDIF()
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_WEBRTC
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_WEBAUDIO
        -DSTARFISH_TIZEN_USERAPP_SDK_API_ONLY
        -DWEBRTC_POSIX
        -DWEBRTC_LINUX
    )
ENDIF()

IF ("${ENABLE_ESPLUSPLAYER}" STREQUAL "1")
    # esplusplayer is a platform-internal Tizen package (Tizen 10+, not in
    # the public app SDK); only meaningful on Tizen platform builds.
    IF (NOT CMAKE_SYSTEM_NAME STREQUAL "Tizen")
        MESSAGE (FATAL_ERROR "ENABLE_ESPLUSPLAYER requires CMAKE_SYSTEM_NAME=Tizen")
    ENDIF()
    MESSAGE (STATUS "esplusplayer MSE backend enabled")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_USE_ESPLUSPLAYER
    )
ENDIF()

IF (${WEBGL} STREQUAL "1")
    MESSAGE (STATUS "WEBGL Experimental Enabled")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_WEBGL
        -DSTARFISH_ENABLE_CANVAS
    )
ENDIF()

IF (${WORKER} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_WORKER
    )

    SET (STARFISH_ENABLE_THREADING ON)
ENDIF()

IF (${SHARED_WORKER} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_WORKER
        -DSTARFISH_ENABLE_SHARED_WORKER
        -DSTARFISH_USE_WORKER_PROCESS
    )
ENDIF()

IF (${SERVICE_WORKER} STREQUAL "1")
    # * SERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
    # : If defined, all service workers use same IPC handle.
    #
    # SERVICE_WORKER_CXXFLAGS will be also used to config the sw host.
    SET (SERVICE_WORKER_CXXFLAGS
        -DSERVICE_WORKER_USE_SINGLE_HOST_CONNECTION
    )
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        ${SERVICE_WORKER_CXXFLAGS}
        -DSTARFISH_ENABLE_WORKER
        -DSTARFISH_ENABLE_SERVICE_WORKER
        -DSTARFISH_USE_WORKER_PROCESS
    )
ENDIF()

IF (${IDB} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_IDB
    )

    SET (STARFISH_ENABLE_THREADING ON)
ENDIF()

IF (${BUILD_CAIRO} STREQUAL "1")
    SET (STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS ${THIRD_PARTY_ROOT}/cairo/out/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER}/include/cairo)
    SET (STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS ${STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/cairo/out/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER}/include)
ELSE()
    SET (STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS)
ENDIF()

IF (${ENABLE_WASM} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM} -DSTARFISH_ENABLE_WASM)
ENDIF()

IF (${ENABLE_DEBUGGER} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM} -DSTARFISH_ENABLE_DEBUGGER)
ENDIF()


IF (${USE_FFMPEG_MEDIA_PLAYER} STREQUAL "1" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_WEBAUDIO
        -DSTARFISH_USE_FFMPEG_MEDIAPLAYER
    )
ELSEIF (NOT CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_USE_MOCK_MEDIAPLAYER
    )
ENDIF()


#######################################################
# CXXFLAGS & LDFLAGS
#######################################################

SET (CXXFLAGS_FROM_ENV $ENV{CXXFLAGS})
SEPARATE_ARGUMENTS(CXXFLAGS_FROM_ENV)
IF (${WEBRTC} STREQUAL "1")
    SET (LWE_CXXFLAGS_DEFAULT -std=c++14 -g3 -fvisibility=hidden -fno-omit-frame-pointer -fstack-protector -fPIC -Wno-deprecated-copy -Wno-invalid-offsetof -Wno-deprecated-declarations)
ELSE()
    SET (LWE_CXXFLAGS_DEFAULT -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fvisibility=hidden -fno-omit-frame-pointer -fstack-protector -fPIC)
ENDIF()

IF (${CMAKE_CXX_COMPILER_ID} MATCHES  "GNU")
    SET (LWE_CXXFLAGS_COMPILER -frounding-math -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-maybe-uninitialized -fsignaling-nans -Wno-aggressive-loop-optimizations -Wno-class-memaccess -Wno-stringop-truncation -Wno-stringop-overflow -Wno-array-bounds -Wno-stringop-overread -Wno-restrict)
ELSEIF (${CMAKE_CXX_COMPILER_ID} MATCHES  "Clang")
    SET (LWE_CXXFLAGS_COMPILER -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee -Wno-expansion-to-defined -Wno-dynamic-class-memaccess -Wno-unused-but-set-variable -Wno-unknown-warning-option -Wno-enum-int-mismatch -Wno-string-concatenation -Wno-inconsistent-missing-override
 -Wno-unused-but-set-parameter -Wno-tautological-pointer-compare -Wno-unused-lambda-capture -Wno-delete-non-abstract-non-virtual-dtor -Wno-array-parameter -Wno-error=character-conversion -Wno-error=unnecessary-virtual-specifier -Wno-nontrivial-memcall)
ENDIF()

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9)
    SET (LWE_CXXFLAGS_COMPILER ${LWE_CXXFLAGS_COMPILER} -Wno-attributes -Wno-deprecated-copy -Wno-cast-function-type -Wno-pessimizing-move -Wno-strict-aliasing -Wno-overloaded-virtual -Wno-mismatched-new-delete -Wno-builtin-macro-redefined)
endif()

#IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen" AND (${CUSTOM} STREQUAL "unified_wearable" OR ${CUSTOM} STREQUAL "prod_wearable"))
#    SET (LWE_CXXFLAGS_MODE -Os)
#ELSE
# ELSE so every non-Debug config gets -O2 -- the previous ELSEIF's exact
# "Release"/"RelWithDebInfo" match left LWE_CXXFLAGS_MODE completely empty
# (no optimization flag at all, not even -O0) for e.g. MinSizeRel.
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET (LWE_CXXFLAGS_MODE -O0)
ELSE()
    SET (LWE_CXXFLAGS_MODE -O2)
ENDIF()

IF (${LTO} STREQUAL "1")
    SET (LWE_CXXFLAGS_LTO -flto)
    SET (LWE_LDFLAGS_LTO -flto)
ENDIF()

IF (${ASAN} STREQUAL "1")
    SET (LWE_CXXFLAGS_ASAN -fsanitize=address)
    SET (LWE_LDFLAGS_ASAN -lasan)
    # SET (LWE_CXXFLAGS_ASAN -fsanitize=thread)
    # SET (LWE_LDFLAGS_ASAN -ltsan)
ENDIF()

IF (${COVERAGE} STREQUAL "1")
    SET (LWE_CXXFLAGS_COVERAGE -fprofile-arcs -ftest-coverage)
    SET (LWE_LDFLAGS_COVERAGE --coverage -lgcov)
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    IF (${BACKEND} STREQUAL "glib_cairo_gl")
        SET (LWE_CXXFLAGS_HOST -Wno-format-nonliteral)
    ENDIF()
    # On Tizen, lto causes GC bug. therefore, force no-lto.
    SET (LWE_CXXFLAGS_FORCE_NOLTO -fno-lto)
    SET (LWE_CFLAGS_FORCE_NOLTO -fno-lto)
    SET (LWE_LDFLAGS_FORCE_NOLTO -fno-lto)
ENDIF()

SET (LWE_CXXFLAGS
    ${LWE_CXXFLAGS_DEFAULT}
    ${LWE_CXXFLAGS_COMPILER}
    ${LWE_CXXFLAGS_HOST}
    ${LWE_CXXFLAGS_BACKEND}
    ${LWE_CXXFLAGS_MODE}
    ${LWE_CXXFLAGS_LTO}
    ${LWE_CXXFLAGS_ASAN}
    ${LWE_CXXFLAGS_COVERAGE}
    ${CXXFLAGS_FROM_ENV}
    ${LWE_CXXFLAGS_FORCE_NOLTO} # Please keep it at the end of the list
)

SET (LDFLAGS_FROM_ENV $ENV{LDFLAGS})
SEPARATE_ARGUMENTS(LDFLAGS_FROM_ENV)

SET (LWE_LDFLAGS_DEFAULT -Wl,--gc-sections -Wl,-rpath=/usr/local/lib -Wl,-rpath='\$\$ORIGIN')
IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET (LWE_LDFLAGS_HOST -L/usr/local/lib -Wl,-rpath=\$$ORIGIN/lib -Wl,-rpath-link=lib)
ELSEIF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    # -rpath-link (unlike -rpath) is link-time-only and can't use the
    # runtime-resolved $ORIGIN token, so it has to be this build's own
    # absolute output dir. Without it, resolving the NEEDED entries of an
    # already-linked shared library (e.g. starfish.executable pulling in
    # the *-impl.so that *-api.so needs) falls back to whatever the
    # cross-ld's own default search behavior is -- newer binutils (Tizen
    # 6.0+'s gcc 9.2 toolchain) also consult -L for that, but the older
    # binutils paired with Tizen 5.x's gcc 6.2.1 toolchain doesn't, and
    # fails with "not found (try using -rpath or -rpath-link)" followed by
    # every symbol the missing .so would have provided coming back
    # undefined.
    SET (LWE_LDFLAGS_HOST -L/usr/local/lib -Wl,-rpath=${LIBDIR}/lwe -Wl,-rpath-link=${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
ENDIF()

SET (LWE_LDFLAGS
    ${LWE_LDFLAGS_DEFAULT}
    ${LWE_LDFLAGS_CUSTOM}
    ${LWE_LDFLAGS_HOST}
    ${LWE_LDFLAGS_LTO}
    ${LWE_LDFLAGS_ASAN}
    ${LDFLAGS_FROM_ENV}
    ${LWE_LDFLAGS_COVERAGE}
    ${LWE_LDFLAGS_CLANG}    
    ${LWE_LDFLAGS_FORCE_NOLTO} # Please keep it at the end of the list
)
#######################################################
# PACKAGES
#######################################################
find_package (PkgConfig REQUIRED)

# OpenSSL on Tizen: 10.0 and below ship OpenSSL 1.1, 10.1 and above ship OpenSSL 3.
# The two -devel packages conflict and both own /usr/include/openssl and
# /usr/lib/lib{ssl,crypto}.so, so the module picked here matches the BuildRequires
# in the spec and is also what the libwebsockets sub-build is told to use
# (see third_party.cmake). Below Tizen 6.0 this must mirror the spec's own
# %else block (pkgconfig(openssl), except 5.5 prod_tv/headless which still use
# pkgconfig(openssl1.1)) -- leaving STARFISH_OPENSSL_MODULE undefined here for
# those versions, as before, made the libwebsockets sub-build fall through to
# its own unhinted find_package(OpenSSL), which fails to find anything in a
# cross-compiled Tizen 5.x sysroot ("Could NOT find OpenSSL").
IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    IF ((${TIZEN_MAJOR_VERSION} GREATER 10) OR ((${TIZEN_MAJOR_VERSION} EQUAL 10) AND (${TIZEN_MINOR_VERSION} GREATER 0)))
        SET (STARFISH_OPENSSL_MODULE openssl3)
    ELSEIF ((${TIZEN_MAJOR_VERSION} GREATER 6) OR (${TIZEN_MAJOR_VERSION} EQUAL 6))
        SET (STARFISH_OPENSSL_MODULE openssl1.1)
    ELSEIF ((${TIZEN_MAJOR_VERSION} EQUAL 5) AND (${TIZEN_MINOR_VERSION} EQUAL 5) AND NOT (${CUSTOM} STREQUAL "prod_tv" OR ${CUSTOM} STREQUAL "headless"))
        SET (STARFISH_OPENSSL_MODULE openssl1.1)
    ELSE()
        SET (STARFISH_OPENSSL_MODULE openssl)
    ENDIF()
    IF (DEFINED STARFISH_OPENSSL_MODULE)
        pkg_check_modules (STARFISH_OPENSSL REQUIRED ${STARFISH_OPENSSL_MODULE})
        # third_party.cmake builds absolute library paths for the libwebsockets
        # sub-build out of the module's own libdir/includedir. Refuse to hand it a
        # "/libssl.so" style path if pkg-config ever reports neither.
        IF (NOT STARFISH_OPENSSL_LIBDIR OR NOT STARFISH_OPENSSL_INCLUDEDIR)
            MESSAGE (FATAL_ERROR "${STARFISH_OPENSSL_MODULE} reports no libdir/includedir: "
                "libdir='${STARFISH_OPENSSL_LIBDIR}' includedir='${STARFISH_OPENSSL_INCLUDEDIR}'")
        ENDIF()
    ENDIF()
ENDIF()

IF (${RUNTIME_ICU} STREQUAL "0")
    pkg_check_modules (STARFISH_THIRD_PARTY_LIBS REQUIRED icu-uc icu-i18n)
    SET (LWE_DEFINES_ICU)
ELSE()
    SET (LWE_DEFINES_ICU -DSTARFISH_ENABLE_RUNTIME_ICU_BINDER)
ENDIF()

IF (${WEBRTC} STREQUAL "1" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules (STARFISH_THIRD_PARTY_LIBS REQUIRED alsa)
ENDIF()

IF (${BACKEND} STREQUAL "glib_cairo_gl" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    # EFL libs are only used by EFL-based shells (sources guarded by
    # STARFISH_SHELL_EFL / STARFISH_SHELL_ECORE_X). Non-EFL shells (e.g. x11)
    # must not require them.
    IF (${SHELL} STREQUAL "efl" OR ${SHELL} STREQUAL "ecore_x" OR ${SHELL} STREQUAL "ecore_wl2")
        # ecore-x only exists on X11 desktops; Tizen (incl. the x86_64
        # emulator) is wayland-only and ships no ecore-x package.
        IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
            pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore ecore-imf ecore-imf-evas glesv2)
        ELSE()
            pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore ecore-x ecore-imf ecore-imf-evas glesv2)
        ENDIF()
    ELSE()
        pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz glesv2)
    ENDIF()
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "glib_headless")
    pkg_check_modules (STARFISH_BACKEND REQUIRED glib-2.0)
ELSEIF (${BACKEND} STREQUAL "glib_cairo_gl" AND CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore ecore-imf)
    pkg_check_modules (STARFISH_BACKEND_ECORE_IMF_EVAS REQUIRED ecore-imf-evas)
    pkg_check_modules (STARFISH_BACKEND_LIBTBM REQUIRED libtbm)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "uv_cairo_gl")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "glib_cairo_gl")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz glib-2.0)
    IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        pkg_check_modules (STARFISH_BACKEND_EGL REQUIRED egl glesv2)
    ELSE()
        pkg_check_modules (STARFISH_BACKEND_EGL REQUIRED egl gles20)
    ENDIF()
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "flutter" AND CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED capi-media-player capi-media-sound-manager freetype2 fontconfig harfbuzz elementary ecore ecore-imf ecore-wl2 wayland-client egl gles20 )
    pkg_check_modules (STARFISH_BACKEND_EGL REQUIRED wayland-client egl)
    pkg_check_modules (STARFISH_BACKEND_ECORE_IMF_EVAS REQUIRED ecore-imf-evas)
    pkg_check_modules (STARFISH_BACKEND_LIBTBM REQUIRED libtbm)
    pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
ENDIF()

IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
    pkg_check_modules (STARFISH_BACKEND_IMAGE REQUIRED libpng)
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    pkg_check_modules (STARFISH_BACKEND_GLES REQUIRED gles20)
    IF (${CUSTOM} STREQUAL "unified_common")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog capi-appfw-app-common capi-media-player capi-network-connection)
    ELSEIF (${CUSTOM} MATCHES "mobile")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog capi-appfw-app-common capi-media-player capi-network-connection capi-media-audio-io)
    ELSEIF (${CUSTOM} MATCHES "wearable")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog capi-appfw-app-common capi-media-player capi-media-sound-manager capi-system-info capi-system-device)
        pkg_check_modules (STARFISH_TIZEN_CUSTOM_BUNDLE REQUIRED bundle)
    ELSEIF (${CUSTOM} STREQUAL "unified_tv")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog capi-appfw-app-common capi-network-connection capi-media-player capi-media-audio-io)
    ELSEIF (${CUSTOM} STREQUAL "headless")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog capi-appfw-app-common capi-network-connection capi-media-player)
    ELSEIF (${CUSTOM} STREQUAL "prod_tv")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog vconf-internal-keys-tv capi-network-connection capi-media-player tts capi-media-audio-io capi-appfw-app-common capi-media-tool capi-system-device)
        pkg_check_modules (STARFISH_TIZEN_CUSTOM_VCONF REQUIRED vconf)
    ELSEIF (${CUSTOM} STREQUAL "flutter")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-appfw-app-common dlog)
    ENDIF()
    IF (${ENABLE_A11Y_TOUCH} STREQUAL "1"
        AND NOT ${CUSTOM} STREQUAL "unified_tv" AND NOT ${CUSTOM} STREQUAL "prod_tv")
        pkg_check_modules (STARFISH_TIZEN_A11Y REQUIRED tts vconf)
        # AT-SPI2 provider prototype (ATK plug registration). Needs the EFL
        # shell (plug id is exposed on the app window evas object).
        IF (${SHELL} STREQUAL "efl")
            pkg_check_modules (STARFISH_TIZEN_A11Y_ATSPI REQUIRED atk atk-bridge-2.0 atspi-2 dbus-1)
            SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
                -DSTARFISH_ENABLE_A11Y_ATSPI
            )
            # Tizen's patched atk adds grab_highlight/clear_highlight to
            # AtkComponentIface (plus the HIGHLIGHTABLE/HIGHLIGHTED states);
            # stock atk does not have them.
            INCLUDE (CheckCSourceCompiles)
            SET (CMAKE_REQUIRED_INCLUDES ${STARFISH_TIZEN_A11Y_ATSPI_INCLUDE_DIRS})
            CHECK_C_SOURCE_COMPILES ("
#include <atk/atk.h>
int main() { AtkComponentIface i; i.grab_highlight = 0; (void)i; return 0; }"
                STARFISH_ATK_HAS_GRAB_HIGHLIGHT_TEST)
            UNSET (CMAKE_REQUIRED_INCLUDES)
            IF (STARFISH_ATK_HAS_GRAB_HIGHLIGHT_TEST)
                SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
                    -DSTARFISH_ATK_HAS_GRAB_HIGHLIGHT
                )
            ENDIF()
        ENDIF()
    ENDIF()
    IF (${WEBRTC} STREQUAL "1")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM_WEBRTC REQUIRED capi-media-player capi-media-sound-manager capi-media-camera capi-media-tool capi-system-device capi-media-audio-io)
    ENDIF()
    IF ("${ENABLE_ESPLUSPLAYER}" STREQUAL "1")
        pkg_check_modules (STARFISH_TIZEN_ESPLUSPLAYER REQUIRED esplusplayer)
    ENDIF()
    # Feature macros decide which platform sources get compiled, so the
    # libraries those sources call have to follow the macros -- not just the
    # ${CUSTOM} profile lists above (which cover only the profiles that
    # historically enabled the feature).
    SET (LWE_DEFINES_ENABLED "${LWE_DEFINES_ARCH};${LWE_DEFINES_CUSTOM}")

    # src/platform/tts/TTSTizen.cpp (and TTSTV.cpp): STARFISH_ENABLE_TTS.
    # prod_tv / the ENABLE_A11Y_TOUCH profiles already ask for tts above; the
    # x86_64 arch defines also enable TTS for e.g. unified_tv on the emulator.
    IF ("${LWE_DEFINES_ENABLED}" MATCHES "STARFISH_ENABLE_TTS")
        pkg_check_modules (STARFISH_TIZEN_TTS REQUIRED tts)
    ENDIF()

    # src/platform/multimedia/MediaPlayerAudioTizen.cpp (audio_out_*):
    # STARFISH_ENABLE_MULTIMEDIA && STARFISH_ENABLE_WEBAUDIO.
    IF ("${LWE_DEFINES_ENABLED}" MATCHES "STARFISH_ENABLE_MULTIMEDIA"
        AND "${LWE_DEFINES_ENABLED}" MATCHES "STARFISH_ENABLE_WEBAUDIO")
        pkg_check_modules (STARFISH_TIZEN_AUDIO_IO REQUIRED capi-media-audio-io)
    ENDIF()
ENDIF()

#######################################################
# LIBRARIES
#######################################################

SET (STARFISH_LIBRARIES_DEFAULT
    ${STARFISH_THIRD_PARTY_LIBS_LIBRARIES}
    pthread
    curl
    ssl
    crypto
    # -lasan # for -fsanitize=address
)

# library for public bridge
IF ("${SHELL}" STREQUAL "x11")
    pkg_check_modules (STARFISH_LIBRARIES_SHELL REQUIRED glib-2.0 x11 egl)
ELSEIF ("${SHELL}" STREQUAL "ecore_x")
    pkg_check_modules (STARFISH_LIBRARIES_SHELL REQUIRED ecore ecore-x ecore-input ecore-imf)
ELSEIF ("${SHELL}" STREQUAL "ecore_wl2")
    pkg_check_modules (STARFISH_LIBRARIES_SHELL REQUIRED ecore ecore-wl2 ecore-input ecore-imf wayland-client)
ELSEIF ("${SHELL}" STREQUAL "tcore_wl")
    pkg_check_modules (STARFISH_LIBRARIES_SHELL REQUIRED tizen-core tizen-core-wl tizen-core-imf glib-2.0 wayland-client)
ELSEIF ("${SHELL}" STREQUAL "efl")
    pkg_check_modules (STARFISH_LIBRARIES_SHELL REQUIRED elementary)
ENDIF()

CHECK_LIBRARY_EXISTS(cap cap_set_flag "" STARFISH_HAVE_LIBCAP)
if (STARFISH_HAVE_LIBCAP)
	list(APPEND STARFISH_LIBRARIES_DEFAULT cap )
endif()

IF (${BACKEND} STREQUAL "glib_cairo_gl" OR ${BACKEND} STREQUAL "flutter" OR ${BACKEND} STREQUAL "uv_cairo_gl")
    IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
        SET (STARFISH_LIBRARIES_BACKEND png_lwe jpeg_lwe gif_lwe webp_lwe z)
    ELSE()
        SET (STARFISH_LIBRARIES_BACKEND jpeg gif)
    ENDIF()

    IF (${USE_CUSTOM_WEBP} STREQUAL "1")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} webp_lwe)
    ELSE()
        IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
            IF ((${TIZEN_MAJOR_VERSION} GREATER 6) OR (${TIZEN_MAJOR_VERSION} EQUAL 6))
                SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} webp)
            ENDIF()
        ELSE()
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} webp)
        ENDIF()
    ENDIF()

    IF (${BACKEND} STREQUAL "glib_cairo_gl")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
    ELSEIF (${BACKEND} STREQUAL "uv_cairo_gl")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} GLESv2)
    ELSEIF (${BACKEND} STREQUAL "glib_cairo_gl")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} glib-2.0)
    ELSEIF (${BACKEND} STREQUAL "flutter" AND CMAKE_SYSTEM_NAME STREQUAL "Tizen")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} wayland-egl)
    ENDIF()
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (STARFISH_LIBRARIES_HOST
        rt
        dl
        capi-location-manager
    )
ENDIF()

IF (${USE_LIBWEBSOCKETS} STREQUAL "1")
    SET (STARFISH_LIBRARIES_HOST ${STARFISH_LIBRARIES_HOST} websockets_lwe)
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    LINK_DIRECTORIES (/usr/local/lib ${OUTPUT_DIRECTORY}/lib)
ELSE()
    LINK_DIRECTORIES (${OUTPUT_DIRECTORY}/lib)
ENDIF()

#######################################################
# INCLUDE DIRS
#######################################################

SET (STARFISH_INCLUDE_DIRS_DEFAULT
   ${STARFISH_ROOT}/src
   ${STARFISH_ROOT}/inc
   ${ESCARGOT_THIRD_PARTY_ROOT}/rapidjson/include
)

IF (${USE_LIBWEBSOCKETS} STREQUAL "1")
    SET (STARFISH_INCLUDE_DIRS_DEFAULT
        ${STARFISH_INCLUDE_DIRS_DEFAULT}
        ${OUTPUT_DIRECTORY}/libwebsockets/include
    )
ENDIF()

IF (${BACKEND} STREQUAL "glib_cairo_gl")
    SET (STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS)
ELSEIF (${BACKEND} STREQUAL "flutter" AND CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS)
ENDIF()

SET (STARFISH_INCLUDE_DIRS_CUSTOM
    ${THIRD_PARTY_ROOT}/MP4Parse/source/include
    ${THIRD_PARTY_ROOT}/webm
)

IF (${WEBRTC} STREQUAL "1")
    SET (STARFISH_WEBRTC_ADDITIONAL_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/webrtc/src/libwebrtc/include
    )
ENDIF()

IF (${WORKER} STREQUAL "1")
    SET (STARFISH_WORKER_ADDITIONAL_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/libtuv/include
        ${THIRD_PARTY_ROOT}/libtuv/src
    )
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (STARFISH_TIZEN_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/deviceapi/src/
        /usr/include/dlog
        /usr/include/location
    )
ENDIF()

IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_USE_EMBEDDED_IMAGE_DECODER
    )
ENDIF()



SET (LWE_DEFINITIONS
    ${LWE_DEFINES_DEFAULT}
    ${LWE_DEFINES_ARCH}
    ${LWE_DEFINES_HOST}
    ${LWE_DEFINES_ICU}
    ${LWE_DEFINES_CUSTOM}
    ${LWE_DEFINES_MODE}
    ${LWE_DEFINES_BACKEND}
    ${STARFISH_SHELL_DEFINES}
)



SET (STARFISH_INCLUDE_ADDITIONAL_DIRS
    ${STARFISH_WAYLAND_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_WEBRTC_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_WORKER_ADDITIONAL_INCLUDE_DIRS}
)
