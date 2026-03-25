SET (OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
MESSAGE(VERBOSE ${OUTPUT_DIRECTORY})

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
EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/
        COMMAND python3 ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/
)

IF (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(LWE_CFLAGS -DGC_DEBUG -g3)
    SET(LWE_CXXFLAGS -DGC_DEBUG -g3)
ELSEIF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    SET(LWE_CFLAGS -DNDEBUG -O1 -g3)
    SET(LWE_CXXFLAGS -DNDEBUG -O1 -g3)
ENDIF()

#######################################################
# GCUTIL
#######################################################

FILE(GLOB GCUTIL_BDWGC_SRC ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil/*.c)
FILE(GLOB GCUTIL_SRC ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil/*.cpp)

SET (GCUTIL_CFLAGS_INTERNAL ${NDK_CFLAGS})
SEPARATE_ARGUMENTS(GCUTIL_CFLAGS_INTERNAL)

SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -g3 -fdata-sections -ffunction-sections -DESCARGOT -fno-strict-aliasing -DGC_DLL=1 -fvisibility=hidden -Wno-unused-variable)
SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DENABLE_DISCLAIM=1 -DGC_ATOMIC_UNCOLLECTABLE=1 -DGC_DONT_REGISTER_MAIN_STATIC_DATA=1 -DGC_ENABLE_SUSPEND_THREAD=1 -DGC_BUILD=1 -DGC_VISIBILITY_HIDDEN_SET=1)
SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DGC_NO_THREADS_DISCOVERY=1 -DGC_VERSION_MAJOR=8 -DGC_VERSION_MICRO=0 -DGC_VERSION_MINOR=3)
SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DHAVE_DLADDR=1 -DHAVE_DLFCN_H=1 -DHAVE_DL_ITERATE_PHDR=1 -DHAVE_INTTYPES_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STDINT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRINGS_H=1)
SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DHAVE_STRING_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1)
SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DIGNORE_DYNAMIC_LOADING=1 -DJAVA_FINALIZATION=1 -DMUNMAP_THRESHOLD=1 -DNO_EXECUTE_PERMISSION=1 -DSTDC_HEADERS=1 -DUSE_MMAP=1 -DUSE_MUNMAP=1)
SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DHAVE_PTHREAD_GETATTR_NP=1 -DUSE_GET_STACKBASE_FOR_MAIN=1)

IF (${ANDROID_SYSROOT_ABI} STREQUAL "arm64" OR ${ANDROID_SYSROOT_ABI} STREQUAL "x86_64")
    SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DESCARGOT_USE_32BIT_IN_64BIT=1)
ENDIF()

IF (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DGC_DEBUG -O0)
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Rel")
    SET (GCUTIL_CFLAGS_INTERNAL ${GCUTIL_CFLAGS_INTERNAL} -DNO_DEBUGGING=1 -O2)
ENDIF()

ADD_LIBRARY (gcutil STATIC ${GCUTIL_BDWGC_SRC} ${GCUTIL_SRC})
TARGET_COMPILE_OPTIONS (gcutil PRIVATE ${GCUTIL_CFLAGS_INTERNAL})
TARGET_INCLUDE_DIRECTORIES (gcutil PRIVATE ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil/include ${STARFISH_ROOT}/third_party/escargot/third_party/GCutil/include/gc)

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

file(GLOB DOUBLEC_SRC "${STARFISH_ROOT}/third_party/escargot/third_party/double_conversion/*.cc" )
file(GLOB YARR_SRC "${STARFISH_ROOT}/third_party/escargot/third_party/yarr/*.cpp" )
file(GLOB XSUM_SRC "${STARFISH_ROOT}/third_party/escargot/third_party/xsum/*.cpp" )
file(GLOB SIMDUTF_SRC "${STARFISH_ROOT}/third_party/escargot/third_party/simdutf/*.cpp" )
IF (ENABLE_RUNTIME_ICU_BINDER)
    file(GLOB RUNTIME_ICU_BINDER_SRC "${STARFISH_ROOT}/third_party/escargot/third_party/runtime_icu_binder/*.cpp" )
ELSE()
    set(RUNTIME_ICU_BINDER_SRC)
ENDIF()
file(GLOB_RECURSE ESCARGOT_SRC "${STARFISH_ROOT}/third_party/escargot/src/**/*.cpp" )
list(REMOVE_ITEM ESCARGOT_SRC "${STARFISH_ROOT}/third_party/escargot/src/shell/Shell.cpp")

# Generate UnicodeIdentifierTables.cpp
MAKE_DIRECTORY(${OUTPUT_DIRECTORY}/escargot_generated/parser)
EXECUTE_PROCESS(
    COMMAND python3 ${ESCARGOT_ROOT}/tools/code_generators/gen_unicode.py --derived_core_properties ${ESCARGOT_ROOT}/tools/unicode_data/DerivedCoreProperties.txt --dst ${OUTPUT_DIRECTORY}/escargot_generated/parser/UnicodeIdentifierTables.cpp
)
SET (ESCARGOT_SRC ${ESCARGOT_SRC} ${OUTPUT_DIRECTORY}/escargot_generated/parser/UnicodeIdentifierTables.cpp)

# Generate YarrCanonicalizeUnicode.cpp
MAKE_DIRECTORY(${OUTPUT_DIRECTORY}/escargot_generated/yarr)
EXECUTE_PROCESS(
    COMMAND python3 ${ESCARGOT_ROOT}/tools/code_generators/generateYarrCanonicalizeUnicode.py ${ESCARGOT_ROOT}/tools/unicode_data/CaseFolding.txt ${OUTPUT_DIRECTORY}/escargot_generated/yarr/YarrCanonicalizeUnicode.cpp
)

FILE(READ ${OUTPUT_DIRECTORY}/escargot_generated/yarr/YarrCanonicalizeUnicode.cpp UNICODE_FILE_CONTENTS)
STRING(REPLACE "config.h" "WTFBridge.h" UNICODE_FILE_CONTENTS "${UNICODE_FILE_CONTENTS}")
STRING(REPLACE "constexpr const" "const" UNICODE_FILE_CONTENTS "${UNICODE_FILE_CONTENTS}")
STRING(REPLACE "constexpr size_t UNICODE" "const size_t UNICODE" UNICODE_FILE_CONTENTS "${UNICODE_FILE_CONTENTS}")
STRING(REPLACE "constexpr CanonicalizationRange unicodeRangeInfo" "const CanonicalizationRange unicodeRangeInfo" UNICODE_FILE_CONTENTS "${UNICODE_FILE_CONTENTS}")
FILE(WRITE ${OUTPUT_DIRECTORY}/escargot_generated/yarr/YarrCanonicalizeUnicode.cpp "${UNICODE_FILE_CONTENTS}")

SET(ESCARGOT_SRC ${ESCARGOT_SRC} ${OUTPUT_DIRECTORY}/escargot_generated/yarr/YarrCanonicalizeUnicode.cpp)

# yarr/UnicodePatternTables.h
EXECUTE_PROCESS(
    COMMAND python3 ${ESCARGOT_ROOT}/tools/code_generators/generateYarrUnicodePropertyTables.py ${ESCARGOT_ROOT}/tools/unicode_data ${OUTPUT_DIRECTORY}/escargot_generated/yarr/UnicodePatternTables.h
)

# YarrCanonicalizeUCS2.cpp
EXECUTE_PROCESS(
    COMMAND python3 ${ESCARGOT_ROOT}/tools/code_generators/generateYarrCanonicalizeUCS2.py ${ESCARGOT_ROOT}/tools/unicode_data/UnicodeData.txt ${OUTPUT_DIRECTORY}/escargot_generated/yarr/YarrCanonicalizeUCS2.cpp
    RESULT_VARIABLE GENERATE_RESULT
    OUTPUT_VARIABLE GENERATE_OUTPUT
    ERROR_VARIABLE GENERATE_ERROR
)

SET(ESCARGOT_SRC ${ESCARGOT_SRC} ${OUTPUT_DIRECTORY}/escargot_generated/yarr/YarrCanonicalizeUCS2.cpp)

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

IF (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    SET(LWE_CFLAGS ${LWE_CFLAGS} -DGC_DEBUG -g3)
    SET(LWE_CXXFLAGS ${LWE_CXXFLAGS} -DGC_DEBUG -g3)
ELSEIF (${CMAKE_BUILD_TYPE} MATCHES "Rel")
    SET(LWE_CFLAGS  ${LWE_CFLAGS} -DNDEBUG -Oz -g3)
    SET(LWE_CXXFLAGS  ${LWE_CXXFLAGS} -DNDEBUG -Oz -g3)
ENDIF()

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
SET(LIBWEBSOCKETS_LOCAL_TARGET ${LIBWEBSOCKETS_BUILD_OUTDIR}/lib/libwebsockets.so)
SET(LIBWEBSOCKETS_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebsockets.so)
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
# LIBBF
#######################################################
file(GLOB LIBBF_SRC "${STARFISH_ROOT}/third_party/escargot/third_party/libbf/*.c" )
ADD_LIBRARY (bf STATIC ${LIBBF_SRC})

#######################################################
# BUILD TARGET
#######################################################

add_library(lwe
        SHARED
        ${STARFISH_SRC}
        ${STARFISH_GEN_SRC}
        ${STARFISH_SUB_SRC}
        ${DOUBLEC_SRC}
        ${YARR_SRC}
        ${XSUM_SRC}
        ${SIMDUTF_SRC}
        ${RUNTIME_ICU_BINDER_SRC}
        ${ESCARGOT_SRC}
        )

target_compile_definitions(lwe PRIVATE ${LWE_DEFINITIONS})
message(fatal_error ${LWE_CXXFLAGS})
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
        gcutil
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
        bf
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
