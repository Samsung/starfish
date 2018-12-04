CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

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

IF (${HOST} STREQUAL "tizen")
SET (OUTPUT_DIRECTORY ${OUTPUT_DIR}/out_tizen/${CUSTOM}/${MODE})
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/)
ELSE()
SET (OUTPUT_DIRECTORY ${OUTPUT_DIR}/out/${MODE})
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/bin)
ENDIF()

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/lib)
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/lib)

#######################################################
# DEFINITION
#######################################################

SET (LWE_DEFINES_DEFAULT
    -DESCARGOT_ENABLE_TYPEDARRAY=1
    -DESCARGOT_ENABLE_PROMISE=1
)

IF (${ARCH} STREQUAL "x64")
    SET (LWE_DEFINES_ARCH
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_INSPECTOR
        -DSTARFISH_ENABLE_DOMPARSER
        -DSTARFISH_ENABLE_TTS
        -DSTARFISH_IGNORE_CROSS_ORIGIN
        -DSTARFISH_ENABLE_HTTPCACHE
    )
ENDIF()

IF (${HOST} STREQUAL "tizen")
    SET (LWE_DEFINES_HOST
        -DSTARFISH_TIZEN
        -DSTARFISH_TIZEN_OBS
        -DSTARFISH_ENABLE_DOMPARSER
        -DSTARFISH_IGNORE_CROSS_ORIGIN
        # -DSTARFISH_ENABLE_MULTIMEDIA
        -DTIZEN_DEVICE_API
        -DSIZE_MAX=0xffffffff
        #-DSTARFISH_IGNORE_SSL_VERIFYPEER
        #-DSTARFISH_ENABLE_INSPECTOR
        #-DSTARFISH_ENABLE_TEST
        #-DSTARFISH_MEDIAPLAYER_DEBUG
    )
ENDIF()

IF (${CUSTOM} STREQUAL "unified_mobile")
    SET (LWE_DEFINES_CUSTOM
        #-DSTARFISH_ENABLE_MULTIMEDIA
    )
ELSEIF (${CUSTOM} STREQUAL "unified_tv")
    SET (LWE_DEFINES_CUSTOM
        #-DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_TIZEN_TV
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        #-DSTARFISH_ENABLE_AVPLAY
        -DSTARFISH_ENABLE_TRANSPARENT_WINDOW
        #-DSTARFISH_ENABLE_TTS
        #-DSTARFISH_ENABLE_BODY_FOCUS_RING
        #-DSTARFISH_ENABLE_VIRTUAL_CURSOR
        #-DUSE_PRODUCT_FEATURE
    )
ELSEIF (${CUSTOM} STREQUAL "prod_tv")
    SET (LWE_DEFINES_CUSTOM
        #-DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_TIZEN_TV
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        #-DSTARFISH_ENABLE_AVPLAY
        -DSTARFISH_ENABLE_TRANSPARENT_WINDOW
        -DSTARFISH_ENABLE_TTS
        #-DSTARFISH_ENABLE_BODY_FOCUS_RING
        #-DSTARFISH_ENABLE_VIRTUAL_CURSOR
        -DUSE_PRODUCT_FEATURE
    )

    IF (NOT ${BACKEND} STREQUAL "dali")
        SET (LWE_DEFINES_CUSTOM ${LWE_DEFINES_CUSTOM} -DSTARFISH_ENABLE_MULTIMEDIA)
    ENDIF()
ELSEIF (${CUSTOM} STREQUAL "unified_wearable")
    SET (LWE_DEFINES_CUSTOM
        #-DSTARFISH_TIZEN_WEARABLE
        -DSTARFISH_TIZEN_WEARABLE_WIDGET
        -DSTARFISH_TIZEN_TRANSPARENT_BACKGROUND
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        -DSTARFISH_DISABLE_OVERFLOW_SCROLL
        -DSTARFISH_ENABLE_MULTIMEDIA
    )
ELSEIF (${CUSTOM} STREQUAL "prod_wearable")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_TIZEN_WEARABLE_WIDGET
        -DSTARFISH_TIZEN_TRANSPARENT_BACKGROUND
        -DSTARFISH_TIZEN_CAPI_LOCATION_MANAGER_ENABLED
        -DSTARFISH_DISABLE_OVERFLOW_SCROLL
        #-DSTARFISH_ENABLE_MULTIMEDIA
    )
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET (LWE_DEFINES_MODE
        -DGC_DEBUG # bdwgc
        -D_GLIBCXX_DEBUG
        -DSTARFISH_ENABLE_TEST
        #-DSTARFISH_ENABLE_NETWORK_TEST
    )
ELSEIF (${MODE} STREQUAL "release")
    SET (LWE_DEFINES_MODE -DNDEBUG)
ELSE()
    MESSAGE (FATAL_ERROR "Release/Debug is NOT SET")
ENDIF()

IF (${BACKEND} STREQUAL "efl_cairo_gl")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL_CAIRO)
ELSEIF (${BACKEND} STREQUAL "dali")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_DALI)
ELSEIF (${BACKEND} STREQUAL "efl_skia")
    IF (${HOST} STREQUAL "linux")
        SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL_SKIA)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "glfw_cairo_gl")
    IF (${HOST} STREQUAL "linux")
        SET (LWE_DEFINES_BACKEND -DSTARFISH_GLFW_CAIRO_GL)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl")
    IF (${HOST} STREQUAL "tizen")
        SET (LWE_DEFINES_BACKEND -DSTARFISH_ECORE_WAYLAND2_CAIRO_GL)
    ENDIF()
ENDIF()

SET (LWE_DEFINITIONS
    ${LWE_DEFINES_DEFAULT}
    ${LWE_DEFINES_ARCH}
    ${LWE_DEFINES_HOST}
    ${LWE_DEFINES_CUSTOM}
    ${LWE_DEFINES_MODE}
    ${LWE_DEFINES_BACKEND}
)

#######################################################
# CXXFLAGS & LDFLAGS
#######################################################

SET (CXXFLAGS_FROM_ENV $ENV{CXXFLAGS})
SEPARATE_ARGUMENTS(CXXFLAGS_FROM_ENV)
SET (LWE_CXXFLAGS_DEFAULT -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-maybe-uninitialized -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fvisibility=hidden -fno-omit-frame-pointer -fstack-protector -fPIC)

IF (${COMPILER} STREQUAL "gcc")
    SET (LWE_CXXFLAGS_COMPILER -frounding-math -fsignaling-nans -Wno-unused-but-set-variable -Wno-unused-but-set-parameter)
ELSEIF (${COMPILER} STREQUAL "clang")
    SET (LWE_CXXFLAGS_COMPILER -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee -stdlib=libc++ -Wno-expansion-to-defined -Wno-dynamic-class-memaccess)
ENDIF()

IF (${HOST} STREQUAL "tizen" AND (${CUSTOM} STREQUAL "unified_wearable" OR ${CUSTOM} STREQUAL "prod_wearable"))
    SET (LWE_CXXFLAGS_MODE -Os)
ELSEIF (${MODE} STREQUAL "debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    SET (LWE_CXXFLAGS_MODE -O0)
ELSEIF (${MODE} STREQUAL "release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    SET (LWE_CXXFLAGS_MODE -O2)
ENDIF()

IF (${HOST} STREQUAL "tizen")
    IF (${BACKEND} MATCHES "efl_cairo" OR ${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali")
        SET (LWE_CXXFLAGS_HOST -Wno-format-nonliteral)
    ENDIF()
ENDIF()

IF (NOT ${BACKEND} STREQUAL "dali")
    SET (LWE_CXXFLAGS_BACKEND -fno-rtti)
ENDIF()

SET (LWE_CXXFLAGS ${LWE_CXXFLAGS_DEFAULT} ${LWE_CXXFLAGS_COMPILER} ${LWE_CXXFLAGS_HOST} ${LWE_CXXFLAGS_BACKEND} ${CXXFLAGS_FROM_ENV} ${LWE_CXXFLAGS_MODE})


SET (LWE_LDFLAGS_DEFAULT -Wl,-rpath=/usr/local/lib)
IF (${HOST} STREQUAL "linux")
    SET (LWE_LDFLAGS_HOST -Wl,--gc-sections -L/usr/local/lib -Wl,-rpath=\$$ORIGIN/lib -Wl,-rpath-link=lib)
ENDIF()

SET (LWE_LDFLAGS ${LWE_LDFLAGS_DEFAULT} ${LWE_LDFLAGS_HOST})
#######################################################
# PACKAGES
#######################################################
find_package (PkgConfig REQUIRED)
pkg_check_modules (STARFISH_THIRD_PARTY_LIBS REQUIRED icu-uc icu-i18n)


IF (${BACKEND} STREQUAL "efl" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} STREQUAL "efl" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf-evas efl-extension)
ELSEIF (${BACKEND} MATCHES "efl_cairo" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} MATCHES "efl_cairo" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf)
    pkg_check_modules (STARFISH_BACKEND_ECORE_IMF_EVAS REQUIRED ecore-imf-evas)
    pkg_check_modules (STARFISH_BACKEND_LIBTBM REQUIRED libtbm)
ELSEIF (${BACKEND} STREQUAL "dali" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore)
ELSEIF (${BACKEND} STREQUAL "dali" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore)
ELSEIF (${BACKEND} STREQUAL "efl_skia" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} STREQUAL "glfw_cairo_gl" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu)
ELSEIF (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" AND ${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf ecore-wl2 wayland-client egl gles20)
    pkg_check_modules (STARFISH_BACKEND_ECORE_IMF_EVAS REQUIRED ecore-imf-evas)
    pkg_check_modules (STARFISH_BACKEND_LIBTBM REQUIRED libtbm)
ENDIF()

IF (${HOST} STREQUAL "tizen")
    IF (${CUSTOM} STREQUAL "unified_common")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-media-player capi-network-connection)
    ELSEIF (${CUSTOM} MATCHES "mobile")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-media-player capi-network-connection)
    ELSEIF (${CUSTOM} MATCHES "wearable")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED dlog capi-media-player capi-media-sound-manager)
        pkg_check_modules (STARFISH_TIZEN_CUSTOM_BUNDLE REQUIRED bundle)
    ELSEIF (${CUSTOM} STREQUAL "unified_tv")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-network-connection capi-media-player)
    ELSEIF (${CUSTOM} STREQUAL "prod_tv")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED vconf-internal-keys-tv capi-network-connection capi-media-player tts)
        pkg_check_modules (STARFISH_TIZEN_CUSTOM_VCONF REQUIRED vconf)
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

IF (${COMPILER} STREQUAL "clang")
    SET (STARFISH_LIBRARIES_COMPILER -stdlib=libc++)
ENDIF()

IF (${BACKEND} MATCHES "efl_cairo" OR ${BACKEND} STREQUAL "efl_skia" OR ${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali")
    SET (STARFISH_LIBRARIES_BACKEND jpeg gif)
    IF (${BACKEND} MATCHES "efl_cairo")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} turbojpeg)
    ELSEIF (${BACKEND} STREQUAL "efl_skia" AND ${ARCH} STREQUAL "x64")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} -Llib turbojpeg skia)
    ELSEIF (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" AND ${HOST} STREQUAL "tizen")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} -Llib/tizen turbojpeg wayland-egl)
    ELSEIF (${BACKEND} STREQUAL "dali" AND ${ARCH} STREQUAL "x64")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} -Llib dali-core dali-adaptor dali-toolkit)
    ELSEIF (${BACKEND} STREQUAL "dali" AND ${HOST} STREQUAL "tizen")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} -Llib/tizen dali-core dali-adaptor dali-toolkit)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "glfw_cairo_gl")
    SET (STARFISH_LIBRARIES_BACKEND GL GLESv2 glfw)
    IF (${ARCH} STREQUAL "x64")
        SET (STARFISH_LIBRARIES_BACKEND ${STARFISH_LIBRARIES_BACKEND} -Llib turbojpeg gif jpeg)
    ENDIF()
ENDIF()

IF (${HOST} STREQUAL "tizen")
    SET (STARFISH_LIBRARIES_HOST
        rt
        dl
        capi-location-manager
    )
    IF (${BACKEND} MATCHES "efl_cairo" OR ${BACKEND} STREQUAL "ecore_wayland2_cairo_gl")
        SET (STARFISH_LIBRARIES_HOST ${STARFISH_LIBRARIES_HOST} -Wl,-soname,liblightweight-web-engine.so.1)
    ELSEIF (${BACKEND} STREQUAL "dali")
        SET (STARFISH_LIBRARIES_HOST ${STARFISH_LIBRARIES_HOST} -Wl,-soname,liblightweight-web-engine-dali-plugin.so.1)
    ENDIF()
ENDIF()

IF (${TOUCH_UI} STREQUAL "1")
    #SET (STARFISH_LIBRARIES_TOUCH_UI capi-location-manager)
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
   ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS}
)

IF (${BACKEND} STREQUAL "dali")
    SET (STARFISH_DALI_ADDITIONAL_INCLUDE_DIRS
        /usr/include/dali
        ${THIRD_PARTY_ROOT}/libtuv/include
        ${THIRD_PARTY_ROOT}/libtuv/src
    )
ELSEIF (${BACKEND} STREQUAL "efl_skia" AND ${ARCH} STREQUAL "x64")
    SET (STARFISH_SKIA_ADDITIONAL_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/android/skia/include
        ${THIRD_PARTY_ROOT}/android/skia/include/effects
        ${THIRD_PARTY_ROOT}/android/skia/include/config
        ${THIRD_PARTY_ROOT}/android/skia/include/core
        ${THIRD_PARTY_ROOT}/android/skia/include/image
        ${THIRD_PARTY_ROOT}/android/skia/include/gpu
        ${THIRD_PARTY_ROOT}/android/skia/include/ports
    )
ELSEIF (${BACKEND} STREQUAL "glfw_cairo_gl" AND ${ARCH} STREQUAL "x64")
    SET (STARFISH_GLFW_ADDITIONAL_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/libtuv/include
        ${THIRD_PARTY_ROOT}/libtuv/src
    )
ELSEIF (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" AND ${HOST} STREQUAL "tizen")
    SET (STARFISH_WAYLAND_ADDITIONAL_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/libtuv/include
        ${THIRD_PARTY_ROOT}/libtuv/src
    )
ELSEIF (${BACKEND} STREQUAL "efl_cairo" OR ${BACKEND} STREQUAL "efl_cairo_gl")
    SET (STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS
    )
ENDIF()

SET (STARFISH_INCLUDE_DIRS_CUSTOM
    ${THIRD_PARTY_ROOT}/MP4Parse/source/include
    ${THIRD_PARTY_ROOT}/webm
)

IF (${TOUCH_UI} STREQUAL "1")
    #SET (STARFISH_TOUCH_UI_ADDITIONAL_INCLUDE_DIRS /usr/include/location)
ENDIF()

IF (${HOST} STREQUAL "tizen")
    SET (STARFISH_TIZEN_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/deviceapi/src/
        /usr/include/dlog
        /usr/include/location
    )
ENDIF()

SET (STARFISH_INCLUDE_ADDITIONAL_DIRS
    ${STARFISH_DALI_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_SKIA_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_GLFW_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_WAYLAND_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_EFL_CAIRO_ADDITIONAL_INCLUDE_DIRS}
    ${STARFISH_TOUCH_UI_ADDITIONAL_INCLUDE_DIRS}
)
