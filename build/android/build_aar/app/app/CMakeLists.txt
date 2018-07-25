# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set (CMAKE_CXX_STANDARD 11)

set (STARFISH_ROOT_PATH ${CMAKE_SOURCE_DIR}/../../../../../ )
set (PREBUILT_LIB_PATH ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a)
set (ANDROID_PLATFORM_ROOT_PATH ${STARFISH_ROOT_PATH}/third_party/android/prebuilt_armv-7a_8.1.0/android )

add_definitions(-DESCARGOT -DHAVE_CONFIG_H -DPLATFORM_ANDROID -DIGNORE_DYNAMIC_LOADING -DGC_DONT_REGISTER_MAIN_STATIC_DATA -DUSE_GET_STACKBASE_FOR_MAIN )
add_definitions(-DSTARFISH_ANDROID -DESCARGOT_ENABLE_TYPEDARRAY -DESCARGOT_ENABLE_PROMISE -DANDROID=1 -DESCARGOT_32=1 -DNDEBUG -DSTARFISH_IGNORE_SSL_VERIFYPEER -DSTARFISH_ENABLE_HTTPCACHE )
#add_definitions(-D_GLIBCXX_DEBUG -DGC_DEBUG )

include_directories(${STARFISH_ROOT_PATH}/third_party/android/prebuilt_armv-7a_8.1.0/include/bdwgc)

include_directories(${STARFISH_ROOT_PATH} ${STARFISH_ROOT_PATH}/inc ${STARFISH_ROOT_PATH}/src)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil/bdwgc/include)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/src)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/include)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/third_party/checked_arithmetic)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/third_party/double_conversion)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/third_party/rapidjson/include)
include_directories(${STARFISH_ROOT_PATH}/third_party/escargot/third_party/yarr)
include_directories(${STARFISH_ROOT_PATH}/third_party/clipper/cpp)
include_directories(${STARFISH_ROOT_PATH}/third_party/rapidxml)
include_directories(${STARFISH_ROOT_PATH}/third_party/webm)
include_directories(${STARFISH_ROOT_PATH}/third_party/libav)
include_directories(${STARFISH_ROOT_PATH}/third_party/MP4Parse/source/include)
include_directories(${STARFISH_ROOT_PATH}/third_party/libav/out/linux/x64/release)
include_directories(${STARFISH_ROOT_PATH}/third_party/zeromq/include)
include_directories(${STARFISH_ROOT_PATH}/third_party/cppzmq)

include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/effects)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/config)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/effects)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/core)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/image)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/gpu)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/skia/include/ports)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/libpng)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/giflib)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/boringssl/src/include)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/libjpeg-turbo)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/curl/include)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/freetype/include)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/harfbuzz_ng/src)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/icu/icu4c/source/common)
include_directories(${ANDROID_PLATFORM_ROOT_PATH}/external/icu/icu4c/source/i18n)

file(GLOB GCUTIL_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil/*.cpp" )
file(GLOB DOUBLEC_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/double_conversion/*.cc" )
file(GLOB YARR_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/yarr/*.cpp" )
file(GLOB BDWGC_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/third_party/GCutil/bdwgc/*.c" )
file(GLOB_RECURSE ESCARGOT_SRC "${STARFISH_ROOT_PATH}/third_party/escargot/src/**/*.cpp" )
file(GLOB_RECURSE STARFISH_SRC "${STARFISH_ROOT_PATH}/src/*.cpp" )

list(REMOVE_ITEM STARFISH_SRC ${STARFISH_ROOT_PATH}/src/shell/shell.cpp)
add_definitions("-fno-rtti
                -Wno-format-nonliteral
                -fsignaling-nans
                -Wno-invalid-offsetof
                -fvisibility=hidden
                -fno-omit-frame-pointer
                -fstack-protector
                -Wno-unused-but-set-variable
                -Wno-unused-but-set-parameter
                -Wno-unused-parameter
                -Wno-unused-result
                -Wno-unused-variable
                -Wno-unused-function
                -Wno-deprecated-declarations
                -Wno-type-limits
                -fno-math-errno
                -fdata-sections
                -ffunction-sections
                -frounding-math ")
add_library( # Sets the name of the library.
             lightweightwebengine
             # Sets the library as a shared library.
             SHARED
             # Provides a relative path to your source file(s).
             ${STARFISH_SRC}
             ${BDWGC_SRC}
             ${GCUTIL_SRC}
             ${DOUBLEC_SRC}
             ${YARR_SRC}
             ${ESCARGOT_SRC}
             )
set_target_properties(lightweightwebengine PROPERTIES LINKER_LANGUAGE CXX)
# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

find_library( z-lib z )
find_library( jnig-lib jnigraphics )

LINK_DIRECTORIES("libs")

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

add_library( ft2-lib SHARED IMPORTED )
set_target_properties( ft2-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libft2.so )

add_library( icu1-lib SHARED IMPORTED )
set_target_properties( icu1-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libicui18n.so )

add_library( icu2-lib SHARED IMPORTED )
set_target_properties( icu2-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libicuuc.so )

add_library( png-lib SHARED IMPORTED )
set_target_properties( png-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libpng.so )

add_library( skia-lib SHARED IMPORTED )
set_target_properties( skia-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libskia.so )

add_library( jpeg-lib SHARED IMPORTED )
set_target_properties( jpeg-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libjpeg.so )

add_library( hb-lib SHARED IMPORTED )
set_target_properties( hb-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libharfbuzz_ng.so )

add_library( curl-lib SHARED IMPORTED )
set_target_properties( curl-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libcurl.so )

add_library( crypto-lib SHARED IMPORTED )
set_target_properties( crypto-lib PROPERTIES IMPORTED_LOCATION ${PREBUILT_LIB_PATH}/libcrypto.so )

target_link_libraries( # Specifies the target library.
                       lightweightwebengine
                       icu1-lib
                       icu2-lib
                       png-lib
                       skia-lib
                       hb-lib
                       ft2-lib
                       jpeg-lib
                       crypto-lib
                       curl-lib
                       ${PREBUILT_LIB_PATH}/libgif.a
                       ${z-lib}
                       ${jnig-lib}
                       ${log-lib} )