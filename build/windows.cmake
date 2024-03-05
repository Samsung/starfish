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
# GLOBAL VARIABLES
#######################################################
IF (${ARCH} STREQUAL "x86")
    SET(WINDOWS_ARCH "Win32")
ELSE()
    MESSAGE(FATAL_ERROR "Unsupported arch")
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET(WINDOWS_MODE "Debug")
ELSE()
    SET(WINDOWS_MODE "Release")
ENDIF()

SET(STARFISH_CXXFLAGS
        /std:c++14
        /Oy-
        /fp:strict
        /Zc:__cplusplus
        /EHs
        /source-charset:utf-8
        /MP
        /wd4244
        /wd4267
        /wd4805
        /wd4018
        /wd4101
        /wd4172
        /wd4305
        /wd4251
    )

IF (${ARCH} STREQUAL "x86")
    SET(STARFISH_CXXFLAGS_ARCH /arch:SSE2)
ELSE()
    SET(STARFISH_CXXFLAGS_ARCH)
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET(STARFISH_CXXFLAGS_MODE /Od /MDd)
ELSE()
    SET(STARFISH_CXXFLAGS_MODE /O2 /MD)
ENDIF()

SET(STARFISH_DEFINES
        -DSTARFISH_WINDOWS
        -DSTARFISH_EXPORTS
        -D_TIMESPEC_DEFINED
        -D_USE_MATH_DEFINES
        -D_WINDOWS
        -D_USRDLL
        -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
        -D_CRT_SECURE_NO_WARNINGS
        -DSTARFISH_ENABLE_MULTIMEDIA
        -DSTARFISH_ENABLE_CANVAS
        -DSTARFISH_ENABLE_OBSOLETE_SPEC
        -DSTARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_LINE_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSFORM_PREFIX
        -DSTARFISH_ENABLE_CSS_WEBKIT_TRANSITION_PREFIX
        -DSTARFISH_ENABLE_ANIMATION
        -DSTARFISH_ENABLE_WEBSOCKET
        -DSTARFISH_IGNORE_CROSS_ORIGIN
        -DSTARFISH_IGNORE_SSL_VERIFYPEER
        -DSTARFISH_BACKEND_STR="windows"
    )

IF (${MODE} STREQUAL "debug")
    SET (STARFISH_DEFINES_MODE
        -DGC_DEBUG # bdwgc
        -D_GLIBCXX_DEBUG
        -DSTARFISH_ENABLE_TEST
    )
ELSEIF (${MODE} STREQUAL "release")
    SET (STARFISH_DEFINES_MODE -DNDEBUG)
ELSE()
    MESSAGE (FATAL_ERROR "Release/Debug is NOT SET")
ENDIF()

SET (STARFISH_INCLUDE_DIRS
    ${STARFISH_ROOT}/inc
    ${STARFISH_ROOT}/src
    ${THIRD_PARTY_ROOT}/escargot/third_party/GCutil
    ${THIRD_PARTY_ROOT}/escargot/third_party/GCutil/bdwgc/include
    ${THIRD_PARTY_ROOT}/escargot/src/api
    ${THIRD_PARTY_ROOT}/clipper/cpp
    ${THIRD_PARTY_ROOT}/skia_matrix
    ${THIRD_PARTY_ROOT}/skia_matrix/include/core
    ${THIRD_PARTY_ROOT}/skia_matrix/include/private
    ${THIRD_PARTY_ROOT}/earcut.hpp/include/mapbox
    ${THIRD_PARTY_ROOT}/MP4Parse/source/include
    ${THIRD_PARTY_ROOT}/escargot/third_party/windows/icu/include
    ${THIRD_PARTY_ROOT}/windows/windows_pthread/include
    ${THIRD_PARTY_ROOT}/windows/curl/include
    ${THIRD_PARTY_ROOT}/windows/cairo/src
    ${THIRD_PARTY_ROOT}/windows/cairo/build/windows/cairo/cairo
    ${THIRD_PARTY_ROOT}/windows/fontconfig
    ${THIRD_PARTY_ROOT}/windows/freetype2/include
    ${THIRD_PARTY_ROOT}/windows/giflib/lib
    ${THIRD_PARTY_ROOT}/windows/libpng
    ${THIRD_PARTY_ROOT}/windows/openssl/win32/include
    ${THIRD_PARTY_ROOT}/escargot/third_party/rapidjson/include
    ${THIRD_PARTY_ROOT}/rapidxml
    ${THIRD_PARTY_ROOT}/robin_map/include
    ${THIRD_PARTY_ROOT}/windows/harfbuzz/src
    ${THIRD_PARTY_ROOT}/webm
    ${THIRD_PARTY_ROOT}/MP4Parse/source
    ${THIRD_PARTY_ROOT}/windows/libwebsockets/build/win32/include
    ${THIRD_PARTY_ROOT}/windows/glew/include
    )

SET (STARFISH_DEPENDENCIES)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES)

#######################################################
# OUTPUT PATH
#######################################################
IF (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    SET (OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out_windows/)
    SET (CMAKE_BINARY_DIR ${OUTPUT_DIRECTORY})
ELSE()
    SET (OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
ENDIF()

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${WINDOWS_MODE})
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${WINDOWS_MODE})
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${WINDOWS_MODE})

#######################################################
# ESCARGOT
#######################################################
SET (ESCARGOT_MODE ${MODE})
SET (ESCARGOT_ARCH ${ARCH})
SET (ESCARGOT_OUTPUT static_lib)

IF (${ENABLE_WASM} STREQUAL "1")
    SET (ESCARGOT_WASM ON)
ENDIF()
IF (${ENABLE_CODECACHE} STREQUAL "1")
    SET (ESCARGOT_CODE_CACHE ON)
ENDIF()

SET (ESCARGOT_HOST ${HOST})

IF (${ENABLE_DEBUGGER} STREQUAL "1")
    SET (ESCARGOT_DEBUGGER ON)
ENDIF()

SET (ESCARGOT_USE_CUSTOM_LOGGING ON)
SET (ESCARGOT_THREADING ON)
SET (ESCARGOT_LIBICU_SUPPORT ON)
SET (ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN OFF)

# ESCARGOT INTERNAL COMPILE OPTION
add_compile_options("-DSCRIPT_FUNCTION_OBJECT_BYTECODE_SIZE_MAX=4194304")
add_compile_options("-DESCARGOT_OBJECT_STRUCTURE_ACCESS_CACHE_BUILD_MIN_SIZE=32")
add_compile_options("-DESCARGOT_OBJECT_STRUCTURE_TRANSITION_MODE_MAX_SIZE=36")
add_compile_options("/MP")

ADD_SUBDIRECTORY (third_party/escargot)

SET (STARFISH_DEPENDENCIES
    ${STARFISH_DEPENDENCIES}
    escargot
)

SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES}
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gc-lib.lib
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libbf.lib
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/escargot.lib)

#######################################################
# JS BINDING
#######################################################
EXECUTE_PROCESS (
    COMMAND python ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/
)
SET (STARFISH_INCLUDE_DIRS
    ${STARFISH_INCLUDE_DIRS}
    ${OUTPUT_DIRECTORY}/starfish_generated/)

#######################################################
# THIRD_PARTY (build outside)
#######################################################

# libpng
SET (LIBPNG_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpng.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpng16.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/zlib1.dll)
ADD_CUSTOM_COMMAND (OUTPUT ${LIBPNG_TARGET}
                    WORKING_DIRECTORY ${THIRD_PARTY_ROOT}/windows/libpng/projects/visualc71/
                    COMMENT "BUILD libpng"
                    COMMAND msbuild libpng.sln /t:libpng  /p:Platform=${ARCH} /p:OutDir=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} /p:IntermediateOutputPath=${OUTPUT_DIRECTORY}/libpng/ /p:Configuration=\"DLL Release\"
)
ADD_CUSTOM_TARGET (libpng
                    DEPENDS ${LIBPNG_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/libpng/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpng.lib)

# libcurl
SET (LIBCURL_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcurl.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcurl.dll)
ADD_CUSTOM_COMMAND (OUTPUT ${LIBCURL_TARGET}
                    WORKING_DIRECTORY ${THIRD_PARTY_ROOT}/windows/curl/projects/Windows/VC15
                    COMMENT "BUILD libcurl"
                    COMMAND msbuild curl-all.sln /t:libcurl /p:Platform=${WINDOWS_ARCH} /p:OutDir=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} /p:IntermediateOutputPath=${OUTPUT_DIRECTORY}/libcurl/ /p:Configuration=\"DLL Release - DLL Windows SSPI\"
)
ADD_CUSTOM_TARGET (libcurl
                    DEPENDS ${LIBCURL_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/curl/include/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcurl.lib)

# giflib
# TODO make this shared
SET (GIFLIB_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/giflib.lib)
ADD_CUSTOM_COMMAND (OUTPUT ${GIFLIB_TARGET}
                    WORKING_DIRECTORY ${THIRD_PARTY_ROOT}/windows/giflib/build/windows/giflib
                    COMMENT "BUILD giflib"
                    COMMAND msbuild giflib.sln /t:giflib /p:Platform=${ARCH} /p:OutDir=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} /p:IntermediateOutputPath=${OUTPUT_DIRECTORY}/giflib/ /p:Configuration=Release
)
ADD_CUSTOM_TARGET (giflib
                    DEPENDS ${GIFLIB_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/giflib/lib/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/giflib.lib)

# harfbuzz
# TODO make this shared
SET (HARFBUZZ_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/harfbuzz.lib)
ADD_CUSTOM_COMMAND (OUTPUT ${HARFBUZZ_TARGET}
                    WORKING_DIRECTORY ${THIRD_PARTY_ROOT}/windows/harfbuzz/build/windows/harfbuzz
                    COMMENT "BUILD harfbuzz"
                    COMMAND msbuild harfbuzz.sln /t:harfbuzz /p:Platform=${WINDOWS_ARCH} /p:OutDir=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} /p:IntermediateOutputPath=${OUTPUT_DIRECTORY}/giflib/ /p:Configuration=Release
)
ADD_CUSTOM_TARGET (harfbuzz
                    DEPENDS ${HARFBUZZ_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/harfbuzz/src/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/harfbuzz.lib)

# cairo(contains freetype, libiconv, fontconfig)
SET (CAIRO_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cairo.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfontconfig.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/freetype.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libiconv.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/freetype.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfontconfig.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cairo.dll)
ADD_CUSTOM_COMMAND (OUTPUT ${CAIRO_TARGET}
                    DEPENDS harfbuzz
                    WORKING_DIRECTORY ${THIRD_PARTY_ROOT}/windows/cairo/build/windows/cairo
                    COMMENT "BUILD cairo"
                    COMMAND msbuild cairo.sln /t:cairo /p:Platform=${ARCH} /p:OutDir=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} /p:IntermediateOutputPath=${OUTPUT_DIRECTORY}/cairo/ /p:Configuration=Release
)
ADD_CUSTOM_TARGET (cairo
                    DEPENDS ${CAIRO_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/fontconfig/)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/freetype2/include/)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/cairo/src/)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/cairo/build/windows/cairo/cairo/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfontconfig.lib)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/freetype.lib)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cairo.lib)

# libwebsockets
# TODO make this shared
SET (LIBWEBSOCKETS_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/websockets_static.lib)
ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_TARGET}
                    WORKING_DIRECTORY ${THIRD_PARTY_ROOT}/windows/libwebsockets/build/${WINDOWS_ARCH}/lib/Release
                    COMMENT "COPY LIBWEBSOCKETS"
                    COMMAND ${CMAKE_COMMAND} -E copy websockets_static.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
)
ADD_CUSTOM_TARGET (libwebsockets
                    DEPENDS ${LIBWEBSOCKETS_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/libwebsockets/build/${WINDOWS_ARCH}/include/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/websockets_static.lib)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ws2_32.lib)

# openssl
SET (OPENSSL_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto-1_1.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl-1_1.dll)
ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_TARGET}
                    COMMENT "COPY OPENSSL"
                    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/openssl/${WINDOWS_ARCH}/bin/libcrypto-1_1.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
                    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/openssl/${WINDOWS_ARCH}/bin/libssl-1_1.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
                    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/openssl/${WINDOWS_ARCH}/lib/libcrypto.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
                    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/openssl/${WINDOWS_ARCH}/lib/libssl.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
)
ADD_CUSTOM_TARGET (openssl
                    DEPENDS ${OPENSSL_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/openssl/${WINDOWS_ARCH}/include/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.lib)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.lib)

# windows_pthread
SET (PTHREAD_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pthreadVC2.dll)
ADD_CUSTOM_COMMAND (OUTPUT ${PTHREAD_TARGET}
                    COMMENT "COPY PTHREAD"
                    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/windows_pthread/dll/${ARCH}/pthreadVC2.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
                    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/windows_pthread/lib/${ARCH}/pthreadVC2.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
)
ADD_CUSTOM_TARGET (windows_pthread
                    DEPENDS ${PTHREAD_TARGET}
)
SET (STARFISH_THIRD_PARTY_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_INCLUDE_DIRS} ${THIRD_PARTY_ROOT}/windows/windows_pthread/include/)
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${THIRD_PARTY_ROOT}/windows/windows_pthread/lib/${ARCH}/pthreadVC2.lib)

# glew
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES}
    ${THIRD_PARTY_ROOT}/windows/glew/lib/${WINDOWS_ARCH}/Release/glew32s.lib
    ${THIRD_PARTY_ROOT}/windows/glew/lib/${WINDOWS_ARCH}/Release/glew.lib)

ADD_CUSTOM_COMMAND(OUTPUT ${THIRD_PARTY_ROOT}/windows/glew/lib/${WINDOWS_ARCH}/Release/glew.dll
    COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/windows/glew/lib/${WINDOWS_ARCH}/Release/glew.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
)
ADD_CUSTOM_TARGET(copy_glewdll
                  COMMENT "COPY glew"
                  DEPENDS ${THIRD_PARTY_ROOT}/windows/glew/lib/${WINDOWS_ARCH}/Release/glew.dll
                  )

SET (STARFISH_DEPENDENCIES
    ${STARFISH_DEPENDENCIES}
    libcurl
    libpng
    giflib
    cairo
    harfbuzz
    libwebsockets
    openssl
    windows_pthread
    copy_glewdll
)

#######################################################
# THIRD_PARTY (build here)
#######################################################
SET (THIRD_PARTY_CXXFLAGS
 /std:c++14 /O2 /Oy- /fp:strict /Zc:__cplusplus /EHs /source-charset:utf-8 /D_CRT_SECURE_NO_WARNINGS /DGC_NOT_DLL /D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING /wd4244 /wd4267 /wd4805 /wd4018 /wd4172
 ${STARFISH_CXXFLAGS_ARCH})
SET (THIRD_PARTY_DEFINITIONS ${STARFISH_DEFINES_MODE})

#######################################################
# SKIA_MATRIX
#######################################################
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_CORE ${THIRD_PARTY_ROOT}/skia_matrix/src/core/*.cpp)
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_PORTS ${THIRD_PARTY_ROOT}/skia_matrix/src/ports/*.cpp)
# TODO make this shared by adding __declspec(dllexport) in third-party source
ADD_LIBRARY (skia_matrix STATIC ${SKIA_MATRIX_SRC_CORE} ${SKIA_MATRIX_SRC_PORTS})
TARGET_INCLUDE_DIRECTORIES (skia_matrix PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix ${THIRD_PARTY_ROOT}/skia_matrix/include/core ${THIRD_PARTY_ROOT}/skia_matrix/include/private)
TARGET_COMPILE_DEFINITIONS (skia_matrix PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (skia_matrix PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/skia_matrix.lib)

#######################################################
# CLIPPER
#######################################################
# TODO make this shared by adding __declspec(dllexport) in third-party source
ADD_LIBRARY (clipper STATIC ${THIRD_PARTY_ROOT}/clipper/cpp/clipper.cpp)
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (clipper PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES (clipper PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (clipper PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/clipper.lib)

#######################################################
# MP4PARSE
#######################################################
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
# TODO make this shared by adding __declspec(dllexport) in third-party source
ADD_LIBRARY (mp4parse STATIC ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/mp4parse.lib)

#######################################################
# WEBM
#######################################################
# TODO make this shared by adding __declspec(dllexport) in third-party source
ADD_LIBRARY (webm STATIC
    ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
    ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
)
TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)
TARGET_COMPILE_DEFINITIONS (webm PRIVATE ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (webm PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/webm.lib)


#######################################################
# Copy libs for MSBuild & link
#######################################################

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/copy_ok.stamp
    DEPENDS third_party/escargot/escargot
    DEPENDS clipper
    DEPENDS mp4parse
    DEPENDS skia_matrix
    DEPENDS webm
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/gc-lib.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/libbf.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/escargot.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/clipper.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/mp4parse.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/skia_matrix.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/webm.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/libbf.lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/copy_ok.stamp
)

ADD_CUSTOM_TARGET(copy_libs
                  COMMENT "COPY LIBS"
                  DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/copy_ok.stamp
                  )

SET (STARFISH_DEPENDENCIES
    ${STARFISH_DEPENDENCIES}
    skia_matrix
    clipper
    mp4parse
    webm
    escargot
)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    SET (STARFISH_DEPENDENCIES
        ${STARFISH_DEPENDENCIES}
        copy_libs
    )
endif()


#######################################################
# STARFISH
#######################################################

FILE (GLOB_RECURSE STARFISH_SRC ${STARFISH_ROOT}/src/*.cpp)

FILE (GLOB_RECURSE STARFISH_SHELL_SRC ${STARFISH_ROOT}/src/shell/*.cpp)
LIST (REMOVE_ITEM STARFISH_SRC ${STARFISH_SHELL_SRC})

LIST (REMOVE_ITEM STARFISH_SRC ${STARFISH_ROOT}/src/platform/public/DeviceInfo.cpp)

FILE (GLOB STARFISH_SRC_GENRATED_BINDING ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/*.cpp)

SET (STARFISH_SRC_LIST
    ${STARFISH_SRC}
    ${STARFISH_SRC_GENRATED_BINDING}
    ${STARFISH_ROOT}/build/windows/winform_bridge/StarFishLoggingAndConsoleBridge.cpp
    ${STARFISH_ROOT}/build/windows/winform_bridge/StarFishWinformBridge.cpp
)

SET (STARFISH_LINK_LIBRARIES clipper escargot mp4parse webm skia_matrix)
SET (STARFISH_LINK_LIBRARIES icu.lib ${STARFISH_THIRD_PARTY_LINK_LIBRARIES})

ADD_LIBRARY (starfish.shared_library SHARED ${STARFISH_SRC_LIST})
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES OUTPUT_NAME "Starfish")
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
TARGET_LINK_LIBRARIES (starfish.shared_library ${STARFISH_LINK_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES (starfish.shared_library PRIVATE ${STARFISH_INCLUDE_DIRS})
TARGET_COMPILE_OPTIONS (starfish.shared_library PRIVATE ${STARFISH_CXXFLAGS} ${STARFISH_CXXFLAGS_MODE} ${STARFISH_CXXFLAGS_ARCH})
TARGET_COMPILE_DEFINITIONS (starfish.shared_library PRIVATE ${STARFISH_DEFINES} ${STARFISH_DEFINES_MODE})
ADD_DEPENDENCIES (starfish.shared_library ${STARFISH_DEPENDENCIES})

IF (CMAKE_GENERATOR MATCHES "Visual Studio")
    ADD_CUSTOM_COMMAND(TARGET starfish.shared_library POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${WINDOWS_MODE}/Starfish.dll ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    )
ENDIF()
