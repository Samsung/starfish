cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
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

#######################################################
# OUTPUT PATH
#######################################################

IF (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    SET (OUTPUT_DIRECTORY ${OUTPUT_DIR}/out/${MODE})
ELSE()
    SET (OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
ENDIF()

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/lib)
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/lib)

IF (${HOST} STREQUAL "tizen") # this needs for gbs build
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
# STARFISH_ENABLE_TTS : enable TTS (Text-To-Speech)
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

IF (${ARCH} STREQUAL "x64")
    SET (LWE_DEFINES_ARCH
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_INSPECTOR
        -DSTARFISH_ENABLE_TTS
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
        -DSTARFISH_ENABLE_WEBSOCKET
        -DSTARFISH_ENABLE_WEBAUDIO
        -DSTARFISH_ENABLE_IDB
    )
ENDIF()

IF (${HOST} STREQUAL "tizen")
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
    IF (${CUSTOM} STREQUAL "prod_tv" OR ${CUSTOM} STREQUAL "unified_tv" OR ${CUSTOM} STREQUAL "unified_mobile")
        SET(LWE_DEFINES_HOST
            ${LWE_DEFINES_HOST}
            -DSTARFISH_ENABLE_WEBSOCKET
        )
    ENDIF()
ENDIF()

IF (${CUSTOM} STREQUAL "unified_mobile")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING
    )
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

    IF (NOT ${BACKEND} STREQUAL "dali")
        SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
            -DSTARFISH_ENABLE_MULTIMEDIA
            -DSTARFISH_ENABLE_WEBAUDIO
        )
    ENDIF()
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
        -DSTARFISH_ENABLE_WEBSOCKET
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
        -DSTARFISH_EFL_HEADLESS
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        #-DSTARFISH_DISABLE_OVERFLOW_SCROLL
        -DSTARFISH_ENABLE_MULTIMEDIA
    )

ENDIF()

IF (${MODE} STREQUAL "debug")
    SET (LWE_DEFINES_MODE
        -DGC_DEBUG # bdwgc
        -D_GLIBCXX_DEBUG
        -DSTARFISH_ENABLE_TEST
    )
ELSEIF (${MODE} STREQUAL "release")
    SET (LWE_DEFINES_MODE -DNDEBUG)
    IF (${ENABLE_TEST} STREQUAL "1")
        SET(LWE_DEFINES_MODE ${LWE_DEFINES_MODE} -DSTARFISH_ENABLE_TEST)
    ENDIF()
ELSE()
    MESSAGE (FATAL_ERROR "Release/Debug is NOT SET")
ENDIF()

IF (${BACKEND} STREQUAL "efl_cairo_gl")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL_CAIRO_GL)
ELSEIF (${BACKEND} STREQUAL "efl_headless")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL_HEADLESS)
ELSEIF (${BACKEND} STREQUAL "dali")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_DALI)
ELSEIF (${BACKEND} STREQUAL "flutter")
    IF (${HOST} STREQUAL "tizen")
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
ELSE ()
    MESSAGE (FATAL_ERROR "BACKEND is NOT SET.(ex. efl_cairo_gl, efl_headless, uv_cairo_gl, dali, flutter)")
ENDIF()

SET (LWE_DEFINES_BACKEND ${LWE_DEFINES_BACKEND}
    -DSTARFISH_BACKEND_STR="${BACKEND}"
)

# Tmp disable WebRTC on Linux until openssl1.1 is installed on all dev machines
IF (${HOST} STREQUAL "linux")
    # SET (WEBRTC "1")
ELSEIF ((${CUSTOM} STREQUAL "unified_tv" OR ((${CUSTOM} STREQUAL "prod_tv") AND (${ENABLE_TEST} STREQUAL "1"))) AND ((${TIZEN_MAJOR_VERSION} GREATER 6) OR (${TIZEN_MAJOR_VERSION} EQUAL 6)))
    IF (NOT ${BACKEND} STREQUAL "dali")
        # SET (WEBRTC "1")
    ENDIF()
ENDIF()

IF (${WEBRTC} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM}
        -DSTARFISH_ENABLE_WEBRTC
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_WEBAUDIO
        -DSTARFISH_ENABLE_WEBSOCKET
        -DSTARFISH_TIZEN_USERAPP_SDK_API_ONLY
        -DWEBRTC_POSIX
        -DWEBRTC_LINUX
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

IF (${BUILD_CAIRO} STREQUAL "1")
    SET (STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS ${THIRD_PARTY_ROOT}/cairo/out/${HOST}/${ARCH}/${MODE}/include/cairo)
    SET (STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS ${STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/cairo/out/${HOST}/${ARCH}/${MODE}/include)
ELSE()
    SET (STARFISH_CAIRO_ADDITIONAL_INCLUDE_DIRS)
ENDIF()

IF (${ENABLE_WASM} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM} -DSTARFISH_ENABLE_WASM)
ENDIF()

IF (${ENABLE_DEBUGGER} STREQUAL "1")
    SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM} -DSTARFISH_ENABLE_DEBUGGER)
ENDIF()


#######################################################
# CXXFLAGS & LDFLAGS
#######################################################

SET (CXXFLAGS_FROM_ENV $ENV{CXXFLAGS})
SEPARATE_ARGUMENTS(CXXFLAGS_FROM_ENV)
IF (${WEBRTC} STREQUAL "1")
    SET (LWE_CXXFLAGS_DEFAULT -std=c++14 -g3 -fvisibility=hidden -fno-omit-frame-pointer -fstack-protector -fPIC -Wno-deprecated-copy -Wno-invalid-offsetof -Wno-deprecated-declarations)
ELSE()
    SET (LWE_CXXFLAGS_DEFAULT -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-maybe-uninitialized -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fvisibility=hidden -fno-omit-frame-pointer -fstack-protector -fPIC)
ENDIF()

IF (${COMPILER} STREQUAL "gcc")
    SET (LWE_CXXFLAGS_COMPILER -frounding-math -fsignaling-nans -Wno-unused-but-set-variable -Wno-unused-but-set-parameter)
ELSEIF (${COMPILER} STREQUAL "clang")
    SET (LWE_CXXFLAGS_COMPILER -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee -stdlib=libc++ -Wno-expansion-to-defined -Wno-dynamic-class-memaccess)
ENDIF()

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9)
    SET (LWE_CXXFLAGS_COMPILER ${LWE_CXXFLAGS_COMPILER} -Wno-attributes -Wno-class-memaccess -Wno-deprecated-copy -Wno-cast-function-type -Wno-stringop-truncation -Wno-pessimizing-move -Wno-strict-aliasing -Wno-stringop-overflow -Wno-overloaded-virtual -Wno-mismatched-new-delete -Wno-builtin-macro-redefined)
endif()

#IF (${HOST} STREQUAL "tizen" AND (${CUSTOM} STREQUAL "unified_wearable" OR ${CUSTOM} STREQUAL "prod_wearable"))
#    SET (LWE_CXXFLAGS_MODE -Os)
#ELSE
IF (${MODE} STREQUAL "debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    SET (LWE_CXXFLAGS_MODE -O0)
ELSEIF (${MODE} STREQUAL "release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    SET (LWE_CXXFLAGS_MODE -O2)
ENDIF()

IF (${LTO} STREQUAL "1")
    SET (LWE_CXXFLAGS_LTO -flto)
    SET (LWE_LDFLAGS_LTO -flto)
ENDIF()

IF (${ASAN} STREQUAL "1")
    SET (LWE_CXXFLAGS_ASAN -fsanitize=address)
    SET (LWE_LDFLAGS_ASAN -lasan)
ENDIF()

IF (${COVERAGE} STREQUAL "1")
    SET (LWE_CXXFLAGS_COVERAGE -fprofile-arcs -ftest-coverage)
    SET (LWE_LDFLAGS_COVERAGE --coverage -lgcov)
ENDIF()

IF (${HOST} STREQUAL "tizen")
    IF (${BACKEND} STREQUAL "efl_cairo_gl" OR ${BACKEND} STREQUAL "dali")
        SET (LWE_CXXFLAGS_HOST -Wno-format-nonliteral)
    ENDIF()
    # On Tizen, lto causes GC bug. therefore, force no-lto.
    SET (LWE_CXXFLAGS_FORCE_NOLTO -fno-lto)
    SET (LWE_CFLAGS_FORCE_NOLTO -fno-lto)
    SET (LWE_LDFLAGS_FORCE_NOLTO -fno-lto)
ENDIF()

IF (NOT ${BACKEND} STREQUAL "dali")
    SET (LWE_CXXFLAGS_BACKEND -fno-rtti)
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
IF (${HOST} STREQUAL "linux")
    SET (LWE_LDFLAGS_HOST -L/usr/local/lib -Wl,-rpath=\$$ORIGIN/lib -Wl,-rpath-link=lib)
ELSEIF (${HOST} STREQUAL "tizen")
    SET (LWE_LDFLAGS_HOST -L/usr/local/lib -Wl,-rpath=${LIBDIR}/lwe)
ENDIF()

SET (LWE_LDFLAGS
    ${LWE_LDFLAGS_DEFAULT}
    ${LWE_LDFLAGS_CUSTOM}
    ${LWE_LDFLAGS_HOST}
    ${LWE_LDFLAGS_LTO}
    ${LWE_LDFLAGS_ASAN}
    ${LDFLAGS_FROM_ENV}
    ${LWE_LDFLAGS_COVERAGE}
    ${LWE_LDFLAGS_FORCE_NOLTO} # Please keep it at the end of the list
)
#######################################################
# PACKAGES
#######################################################
find_package (PkgConfig REQUIRED)

IF (${RUNTIME_ICU} STREQUAL "0")
    pkg_check_modules (STARFISH_THIRD_PARTY_LIBS REQUIRED icu-uc icu-i18n)
    SET (LWE_DEFINES_ICU)
ELSE()
    SET (LWE_DEFINES_ICU -DSTARFISH_ENABLE_RUNTIME_ICU_BINDER)
ENDIF()


IF (${WEBRTC} STREQUAL "1" AND ${HOST} STREQUAL "linux")
    pkg_check_modules (STARFISH_THIRD_PARTY_LIBS REQUIRED alsa)
ENDIF()

IF (${BACKEND} STREQUAL "efl_cairo_gl" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore ecore-x ecore-imf ecore-imf-evas glesv2)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "efl_headless" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED elementary ecore ecore-x ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} STREQUAL "efl_cairo_gl" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore ecore-imf)
    pkg_check_modules (STARFISH_BACKEND_ECORE_IMF_EVAS REQUIRED ecore-imf-evas)
    pkg_check_modules (STARFISH_BACKEND_LIBTBM REQUIRED libtbm)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "dali" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "dali" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz elementary ecore)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "uv_cairo_gl")
    pkg_check_modules (STARFISH_BACKEND REQUIRED freetype2 fontconfig harfbuzz)
    IF (${BUILD_CAIRO} STREQUAL "0")
        pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "flutter" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED capi-media-player capi-media-sound-manager freetype2 fontconfig harfbuzz elementary ecore ecore-imf ecore-wl2 wayland-client egl gles20 )
    pkg_check_modules (STARFISH_BACKEND_EGL REQUIRED wayland-client egl)
    pkg_check_modules (STARFISH_BACKEND_ECORE_IMF_EVAS REQUIRED ecore-imf-evas)
    pkg_check_modules (STARFISH_BACKEND_LIBTBM REQUIRED libtbm)
    pkg_check_modules (STARFISH_BACKEND_CAIRO REQUIRED cairo)
ENDIF()

IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
    pkg_check_modules (STARFISH_BACKEND_IMAGE REQUIRED libpng)
ENDIF()

IF (${HOST} STREQUAL "tizen")
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
    IF (${WEBRTC} STREQUAL "1")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM_WEBRTC REQUIRED capi-media-player capi-media-sound-manager capi-media-camera capi-media-tool capi-system-device capi-media-audio-io)
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

CHECK_LIBRARY_EXISTS(cap cap_set_flag "" STARFISH_HAVE_LIBCAP)
if (STARFISH_HAVE_LIBCAP)
	list(APPEND STARFISH_LIBRARIES_DEFAULT cap )
endif()

IF (${COMPILER} STREQUAL "clang")
    SET (STARFISH_LIBRARIES_COMPILER -stdlib=libc++)
ENDIF()

IF (${BACKEND} STREQUAL "efl_cairo_gl" OR ${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "flutter" OR ${BACKEND} STREQUAL "uv_cairo_gl")
    IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
        SET (STARFISH_LIBRARIES_BACKEND png_lwe jpeg_lwe gif_lwe z)
    ELSE()
        SET (STARFISH_LIBRARIES_BACKEND jpeg gif)
    ENDIF()

    IF (${USE_CUSTOM_WEBP} STREQUAL "1")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} webp_lwe)
    ELSE()
        IF (${HOST} STREQUAL "tizen")
            IF ((${TIZEN_MAJOR_VERSION} GREATER 6) OR (${TIZEN_MAJOR_VERSION} EQUAL 6))
                SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} webp)
            ENDIF()
        ELSE()
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} webp)
        ENDIF()
    ENDIF()

    IF (${BACKEND} STREQUAL "efl_cairo_gl")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
    ELSEIF (${BACKEND} STREQUAL "uv_cairo_gl")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} GLESv2)
    ELSEIF (${BACKEND} STREQUAL "flutter" AND ${HOST} STREQUAL "tizen")
        IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "0")
            SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
        ENDIF()
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} wayland-egl)
    ELSEIF (${BACKEND} STREQUAL "dali" AND ${ARCH} STREQUAL "x64" AND ((${TIZEN_MAJOR_VERSION} LESS 5) OR (${TIZEN_MAJOR_VERSION} EQUAL 5)))
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} dali-core dali-adaptor dali-toolkit)
    ELSEIF (${BACKEND} STREQUAL "dali" AND ${HOST} STREQUAL "tizen" AND ((${TIZEN_MAJOR_VERSION} LESS 5) OR (${TIZEN_MAJOR_VERSION} EQUAL 5)))
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} dali-core dali-adaptor dali-toolkit)
    ENDIF()
ENDIF()

IF (${HOST} STREQUAL "tizen")
    SET (STARFISH_LIBRARIES_HOST
        rt
        dl
        capi-location-manager
    )
ENDIF()

IF (${CUSTOM} STREQUAL "prod_tv" OR ${CUSTOM} STREQUAL "unified_tv" OR ${CUSTOM} STREQUAL "unified_mobile")
    SET (STARFISH_LIBRARIES_HOST ${STARFISH_LIBRARIES_HOST} websockets_lwe)
ENDIF()

IF (${HOST} STREQUAL "linux")
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
   ${OUTPUT_DIRECTORY}/libwebsockets/include
)

IF (${BACKEND} STREQUAL "dali")
    SET (STARFISH_DALI_ADDITIONAL_INCLUDE_DIRS
        /usr/include/dali
    )
ELSEIF (${BACKEND} STREQUAL "efl_cairo_gl")
    SET (STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS)
ELSEIF (${BACKEND} STREQUAL "flutter" AND ${HOST} STREQUAL "tizen")
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

IF (${HOST} STREQUAL "tizen")
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
)



SET (STARFISH_INCLUDE_ADDITIONAL_DIRS
    ${STARFISH_DALI_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_WAYLAND_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_WEBRTC_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_WORKER_ADDITIONAL_INCLUDE_DIRS}
)
