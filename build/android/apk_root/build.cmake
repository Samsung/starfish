cmake_minimum_required(VERSION 3.4.1)

project (STARFISH)

#######################################################
# CONFIGURATION
#######################################################

set(CMAKE_CXX_STANDARD 11)
set(OPENGL_LIB GLESv3)
set(LTO "0" CACHE STRING "LTO")
set(STARFISH_ANDROID_OS "9" CACHE STRING "ANDROID_OS")


#######################################################
# SOURCE FILES
#######################################################

file(GLOB_RECURSE STARFISH_SRC "${STARFISH_ROOT_PATH}/src/*.cpp" )
list(REMOVE_ITEM STARFISH_SRC "${STARFISH_ROOT_PATH}/src/shell/shell.cpp")

file(GLOB CLIPPER_SRC "${STARFISH_ROOT_PATH}/third_party/clipper/cpp/clipper.cpp" )
file(GLOB GCUTIL_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil/*.cpp" )
file(GLOB DOUBLEC_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/double_conversion/*.cc" )
file(GLOB YARR_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/yarr/*.cpp" )
file(GLOB BDWGC_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil/bdwgc/*.c" )
file(GLOB_RECURSE ESCARGOT_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/src/**/*.cpp" )


#######################################################
# INCLUDE DIRS
#######################################################

IF (${STARFISH_ANDROID_OS} STREQUAL "9")
    set(ANDROID_PLATFORM_ROOT_PATH ${STARFISH_ROOT_PATH}/third_party/android/prebuilt_armv-7a_8.1.0 )
ELSEIF(${STARFISH_ANDROID_OS} STREQUAL "10")
    set(ANDROID_PLATFORM_ROOT_PATH ${STARFISH_ROOT_PATH}/third_party/android/prebuilt_10.0 )
ENDIF()

set(LWE_INCLUDE_DIRS
    ${ANDROID_PLATFORM_ROOT_PATH}/include/bdwgc
    ${STARFISH_ROOT_PATH} ${STARFISH_ROOT_PATH}/inc ${STARFISH_ROOT_PATH}/src
    ${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil/bdwgc/include
    ${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil
    ${STARFISH_ROOT_PATH}/third_party/escargot/src
    ${STARFISH_ROOT_PATH}/third_party/escargot/src/api
    ${STARFISH_ROOT_PATH}/third_party/escargot/include
    ${STARFISH_ROOT_PATH}/third_party/escargot/third_party/checked_arithmetic
    ${STARFISH_ROOT_PATH}/third_party/escargot/third_party/double_conversion
    ${STARFISH_ROOT_PATH}/third_party/escargot/third_party/rapidjson/include
    ${STARFISH_ROOT_PATH}/third_party/escargot/third_party/yarr
    ${STARFISH_ROOT_PATH}/third_party/clipper/cpp
    ${STARFISH_ROOT_PATH}/third_party/rapidxml
    ${STARFISH_ROOT_PATH}/third_party/earcut.hpp/include/mapbox
    ${STARFISH_ROOT_PATH}/third_party/third_party/clipper/cpp
    ${STARFISH_ROOT_PATH}/third_party/libtuv/include
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/effects
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/config
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/effects
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/core
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/image
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/gpu
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/skia/include/ports
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/libpng
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/giflib
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/boringssl/src/include
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/libjpeg-turbo
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/curl/include
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/freetype/include
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/harfbuzz_ng/src
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/icu/icu4c/source/common
    ${ANDROID_PLATFORM_ROOT_PATH}/android/external/icu/icu4c/source/i18n
)


#######################################################
# DEFINITION
#######################################################

set(LWE_DEFINITIONS
    -DHAVE_CONFIG_H
    -DANDROID
    -DPLATFORM_ANDROID
    -DIGNORE_DYNAMIC_LOADING
    -DGC_DONT_REGISTER_MAIN_STATIC_DATA
    -DUSE_GET_STACKBASE_FOR_MAIN
    -DSTARFISH_ANDROID
    -DSTARFISH_ENABLE_DOMPARSER
    -DSTARFISH_ENABLE_HTTPCACHE
    -DSTARFISH_IGNORE_SSL_VERIFYPEER
    -DESCARGOT
    -DESCARGOT_ENABLE_TYPEDARRAY
    -DESCARGOT_ENABLE_PROMISE)

IF (${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
        -DESCARGOT_64=1)
ELSEIF(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(LWE_DEFINITIONS "${LWE_DEFINITIONS}"
        -DESCARGOT_32=1)
ENDIF()


#######################################################
# CXXFLAGS & LDFLAGS
#######################################################

set(LWE_CXXFLAGS
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
    -fno-rtti
    -fexceptions
    -fvisibility=hidden
    -fno-omit-frame-pointer
    -fstack-protector
    -fno-math-errno
    -fdata-sections
    -ffunction-sections
    -fno-fast-math
    -fno-unsafe-math-optimizations
    -fdenormal-fp-math=ieee)

set(LWE_LDFLAGS )

IF (${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(LWE_CXXFLAGS "${LWE_CXXFLAGS}"
        -march=armv8-a)
ELSEIF(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(LWE_CXXFLAGS "${LWE_CXXFLAGS}"
        -march=armv7-a
        -mfloat-abi=softfp
        -mfpu=neon)
ENDIF()

IF (${LTO} STREQUAL "1")
    # armeabi-v7a uses -Os by default, which cannot be used with -flto
    IF (${ANDROID_ABI} STREQUAL "arm64-v8a")
        set(LWE_CXXFLAGS "${LWE_CXXFLAGS}" -flto)
        set(LWE_LDFLAGS "${LWE_LDFLAGS}" -flto)
    ENDIF()
ENDIF()


#######################################################
# LIBRARIES
#######################################################

IF (${ANDROID_ABI} STREQUAL "arm64-v8a")
    set (PACKAGED_LIB_PATH ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a)
    set (PREBUILT_LIB_PATH ${ANDROID_PLATFORM_ROOT_PATH}/lib/arm64-v8a)
ELSEIF(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set (PACKAGED_LIB_PATH ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a)
    set (PREBUILT_LIB_PATH ${ANDROID_PLATFORM_ROOT_PATH}/lib/armeabi-v7a)
ENDIF()

# Copy prebuilt library to packged lib path.
execute_process(COMMAND @rm ${PACKAGED_LIB_PATH}/*)

file(GLOB PREBUILT_SHARED_LIBS
  "${PREBUILT_LIB_PATH}/*.so"
)
list(REMOVE_ITEM PREBUILT_SHARED_LIBS
    ${PREBUILT_LIB_PATH}/libicui18n.so
    ${PREBUILT_LIB_PATH}/libicuuc.so
)

file(COPY ${PREBUILT_SHARED_LIBS}
     DESTINATION ${PACKAGED_LIB_PATH})

find_library( log-lib log )
find_library( z-lib z )
find_library( jnig-lib jnigraphics )
find_library( android-lib android )

LINK_DIRECTORIES("libs")

add_library( ft2-lib SHARED IMPORTED )
set_target_properties( ft2-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libft2.so )

add_library( png-lib SHARED IMPORTED )
set_target_properties( png-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libpng.so )

add_library( skia-lib SHARED IMPORTED )
set_target_properties( skia-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libskia_shared.so )
#set_target_properties( skia-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libhwui.so )

add_library( jpeg-lib SHARED IMPORTED )
set_target_properties( jpeg-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libjpeg.so )

add_library( hb-lib SHARED IMPORTED )
set_target_properties( hb-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libharfbuzz_ng.so )

add_library( curl-lib SHARED IMPORTED )
set_target_properties( curl-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libcurl.so )

add_library( crypto-lib SHARED IMPORTED )
set_target_properties( crypto-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libcrypto.so )

add_library( icui18n-lib SHARED IMPORTED )
set_target_properties( icui18n-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libicui18n.so )

add_library( icuuc-lib SHARED IMPORTED )
set_target_properties( icuuc-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libicuuc.so )

#######################################################
# BUILD TARGET
#######################################################

add_library(lightweightwebengine
            SHARED
            ${STARFISH_SRC}
            ${BDWGC_SRC}
            ${GCUTIL_SRC}
            ${DOUBLEC_SRC}
            ${YARR_SRC}
            ${ESCARGOT_SRC}
            ${CLIPPER_SRC})

target_compile_definitions(lightweightwebengine PUBLIC ${LWE_DEFINITIONS})
target_compile_options(lightweightwebengine PUBLIC ${LWE_CXXFLAGS})
target_include_directories(lightweightwebengine PUBLIC ${LWE_INCLUDE_DIRS})
target_link_libraries(lightweightwebengine ${LWE_LDFLAGS})
set_target_properties(lightweightwebengine PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries( lightweightwebengine
                       png-lib
                       skia-lib
                       hb-lib
                       ft2-lib
                       jpeg-lib
                       curl-lib
                       crypto-lib
                       icui18n-lib
                       icuuc-lib
                       ${PREBUILT_LIB_PATH}/libgif.a
                       ${PREBUILT_LIB_PATH}/libtuv.a
                       ${z-lib}
                       ${jnig-lib}
                       ${log-lib}
                       ${OPENGL_LIB}
                       ${android-lib}
                       EGL)
