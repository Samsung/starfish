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

IF (${ARCH} STREQUAL "tizen")
SET (OUTPUT_DIRECTORY ${OUTPUT_DIR}/out_tizen/${CUSTOM}/${MODE} CACHE STRING "OUTPUT DIRECTORY")
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/)
ELSE()
SET (OUTPUT_DIRECTORY ${OUTPUT_DIR}/out/${MODE} CACHE STRING "OUTPUT DIRECTORY")
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
ELSEIF (${ARCH} STREQUAL "tizen")
    SET (LWE_DEFINES_ARCH
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

IF (${CUSTOM} STREQUAL "unified")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_ENABLE_MULTIMEDIA
    )
ELSEIF (${CUSTOM} STREQUAL "vd")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_TIZEN_TV
        #-DSTARFISH_ENABLE_AVPLAY
        -DSTARFISH_ENABLE_TRANSPARENT_WINDOW
        -DSTARFISH_ENABLE_TTS
        #-DSTARFISH_ENABLE_BODY_FOCUS_RING
        #-DSTARFISH_ENABLE_VIRTUAL_CURSOR
        -DUSE_PRODUCT_FEATURE
    )
ELSEIF (${CUSTOM} STREQUAL "im")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_TIZEN_WEARABLE
        # -DSTARFISH_TIZEN_WEARABLE_WIDGET
        -DSTARFISH_TIZEN_TRANSPARENT_BACKGROUND
        -DSTARFISH_ENABLE_MULTIMEDIA
    )
ELSEIF (${CUSTOM} STREQUAL "da")
    # NOTHING
ELSEIF (${CUSTOM} STREQUAL "headless" AND ${ARCH} STREQUAL "tizen")
    SET (LWE_DEFINES_CUSTOM
        -DSTARFISH_TIZEN_HEADLESS
        -DSTARFISH_ENABLE_SHELL
        # -DSTARFISH_ENABLE_CANVAS
    )
ENDIF()

IF (${MODE} STREQUAL "debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    SET (LWE_DEFINES_MODE
        -DGC_DEBUG # bdwgc
        -D_GLIBCXX_DEBUG
        -DSTARFISH_ENABLE_TEST
        #-DSTARFISH_ENABLE_NETWORK_TEST
    )
ELSEIF (${MODE} STREQUAL "release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    SET (LWE_DEFINES_MODE -DNDEBUG)
ENDIF()

IF (${BACKEND} STREQUAL "efl")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL)
ELSEIF (${BACKEND} STREQUAL "efl_cairo")
    IF (${TOUCH_UI} STREQUAL "0")
        SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL_CAIRO_HEADLESS)
    ELSE()
        SET (LWE_DEFINES_BACKEND -DSTARFISH_EFL_CAIRO)
    ENDIF()
ELSEIF (${BACKEND} STREQUAL "dali")
    SET (LWE_DEFINES_BACKEND -DSTARFISH_DALI -DGC_THREADS)
ENDIF()

SET (LWE_DEFINITIONS
    ${LWE_DEFINES_DEFAULT}
    ${LWE_DEFINES_ARCH}
    ${LWE_DEFINES_CUSTOM}
    ${LWE_DEFINES_MODE}
    ${LWE_DEFINES_BACKEND}
)

#######################################################
# CXXFLAGS
#######################################################

SET (LWE_CXXFLAGS_DEFAULT -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fvisibility=hidden -fno-omit-frame-pointer -fstack-protector -fPIC)

IF (${COMPILER} STREQUAL "gcc")
    SET (LWE_CXXFLAGS_COMPILER -frounding-math -fsignaling-nans -Wno-unused-but-set-variable -Wno-unused-but-set-parameter)
ELSEIF (${COMPILER} STREQUAL "clang")
    SET (LWE_CXXFLAGS_COMPILER -fno-fast-math -fno-unsafe-math-optimizations -fdenormal-fp-math=ieee -stdlib=libc++ -Wno-expansion-to-defined -Wno-dynamic-class-memaccess)
ENDIF()

IF (${ARCH} STREQUAL "tizen" AND ${CUSTOM} STREQUAL "gear")
    SET (LWE_CXXFLAGS_MODE -Os)
ELSEIF (${MODE} STREQUAL "debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    SET (LWE_CXXFLAGS_MODE -O0)
ELSEIF (${MODE} STREQUAL "release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    SET (LWE_CXXFLAGS_MODE -O2)
ENDIF()

IF (${HOST} STREQUAL "linux")
    SET (LWE_CXXFLAGS_HOST -fno-rtti -fvisibility=hidden)
    SET (LWE_LDFLAGS "-Wl,--gc-sections -L/usr/local/lib -Wl,-rpath=\$$ORIGIN/lib -Wl,-rpath-link=lib")
ELSEIF (${ARCH} STREQUAL "tizen")
    SET (LWE_CXXFLAGS_HOST -fno-rtti -Wno-format-nonliteral)
    IF (${CUSTOM} STREQUAL "speaker")
        SET (LWE_CXXFLAGS_HOST ${LWE_CXXFLAGS_HOST} -marm)
    ENDIF()
ENDIF()

SET (LWE_CXXFLAGS ${LWE_CXXFLAGS_DEFAULT} ${LWE_CXXFLAGS_COMPILER} ${LWE_CXXFLAGS_MODE} ${LWE_CXXFLAGS_HOST})

#######################################################
# PACKAGES
#######################################################
find_package (PkgConfig REQUIRED)
pkg_check_modules (STARFISH_THIRD_PARTY_LIBS REQUIRED icu-uc icu-i18n)


IF (${BACKEND} STREQUAL "efl" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} STREQUAL "efl" AND ${ARCH} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf-evas efl-extension)
ELSEIF (${BACKEND} STREQUAL "efl_cairo" AND ${ARCH} STREQUAL "x64")
    pkg_check_modules (STARFISH_BACKEND REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-x ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} STREQUAL "efl_cairo" AND ${ARCH} STREQUAL "tizen")
    pkg_check_modules (STARFISH_BACKEND REQUIRED dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore ecore-imf ecore-imf-evas)
ELSEIF (${BACKEND} STREQUAL "efl_cairo" AND ${ARCH} STREQUAL "tizen" AND ${CUSTOM} STREQUAL "headless")
    pkg_check_modules (STARFISH_BACKEND REQUIRED dlog ecore)
ELSEIF (${BACKEND} STREQUAL "dali" AND ${ARCH} STREQUAL "x64")
    pkg_check_moudles (STARFISH_BACKEDN REQUIRED libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore)
ELSEIF (${BACKEND} STREQUAL "dali" AND ${ARCH} STREQUAL "tizen")
    pkg_check_moudles (STARFISH_BACKEDN REQUIRED dlog libpng cairo freetype2 fontconfig harfbuzz harfbuzz-icu elementary ecore)
ENDIF()

IF (${ARCH} STREQUAL "tizen")
    IF (${CUSTOM} STREQUAL "unified")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-media-player capi-network-connection)
    ELSEIF (${CUSTOM} STREQUAL "gear")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-media-player bundle capi-network-connection)
    ELSEIF (${CUSTOM} STREQUAL "vd")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED vconf vconf-internal-keys-tv vd-win-util capi-network-connection capi-media-player)
    ELSEIF (${CUSTOM} STREQUAL "speaker")
        pkg_check_modules (STARFISH_TIZEN_CUSTOM REQUIRED capi-network-connection capi-media-player)
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

IF (${BACKEND} STREQUAL "efl_cairo" OR ${BACKEND} STREQUAL "dali")
    SET (STARFISH_LIBRARIES_BACKEND jpeg gif)
ENDIF()

IF (${ARCH} STREQUAL "tizen")
    SET (STARFISH_LIBRARIES_ARCH
        rt
        dl
        capi-location-manager
        -Wl,-soname,liblightweight-web-engine.so
    )
ELSE()
    SET (STARFISH_LIBRARIES_ARCH zmq)
ENDIF()

IF (${TOUCH_UI} STREQUAL "1")
#    SET (STARFISH_LIBRARIES_TOUCH_UI capi-location-manager)
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
   ${THIRD_PARTY_LIBS_INCLUDE_DIRS}
)

IF (${CUSTOM} STREQUAL "vd")
    SET (STARFISH_INCLUDE_DIRS_CUSTOM ${STARFISH_ROOT}/src/platform/lwe_vd)
ELSEIF (${CUSTOM} STREQUAL "im")
    SET (STARFISH_INCLUDE_DIRS_CUSTOM ${STARFISH_ROOT}/src/platform/lwe_im)
ELSEIF (${CUSTOM} STREQUAL "da")
    SET (STARFISH_INCLUDE_DIRS_CUSTOM ${STARFISH_ROOT}/src/platform/lwe_da)
ENDIF()

IF (${BACKEND} STREQUAL "dali")
    SET (STARFISH_DALI_ADDTIONAL_INCLUDE_DIRS /usr/include/dali ${THIRD_PARTY_ROOT}/libtuv/include ${THIRD_PARTY_ROOT}/libtuv/src)
    IF (${ARCH} STREQUAL "x64")
        SET (STARFISH_DALI_ADDTIONAL_LIBRARIES -Llib tuv dali-core dali-adaptor dali-toolkit)
    ELSEIF (${ARCH} STREQUAL "tizen")
        SET (STARFISH_DALI_ADDTIONAL_LIBRARIES -Llib/tizen tuv dali-core dali-adaptor dali-toolkit)
    ENDIF()
ENDIF()

IF (${ARCH} STREQUAL "tizen" OR ${TOUCH_UI} STREQUAL "1")
    SET (STARFISH_INCLUDE_ADDITIONAL /usr/include/location)
ENDIF()

IF (${ARCH} STREQUAL "tizen")
    SET (STARFISH_TIZEN_INCLUDE_DIRS
        ${THIRD_PARTY_ROOT}/deviceapi/src/
        /usr/include/dlog
        /usr/include/location
        /usr/include/media
    )
ENDIF()
