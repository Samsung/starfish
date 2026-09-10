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
IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    SET (WINDOWS_ARCH "Win32")
ELSEIF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x64")
    SET (WINDOWS_ARCH "x64")
ELSE()
    MESSAGE (FATAL_ERROR "Windows supports only Intel x86 and x86_64")
ENDIF()

SET(STARFISH_CXXFLAGS
        /std:c++14
        /Oy-
        /fp:strict
        /Zc:__cplusplus
        /EHs
        /source-charset:utf-8
        /execution-charset:utf-8
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

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    SET(STARFISH_CXXFLAGS_ARCH /arch:SSE2)
ELSE()
    SET(STARFISH_CXXFLAGS_ARCH)
ENDIF()

# Generator expressions instead of a configure-time IF(CMAKE_BUILD_TYPE ...):
# CMAKE_BUILD_TYPE is empty at configure time for multi-config generators
# (Visual Studio) -- an IF here would always take the ELSE branch regardless
# of which config is actually selected via --config at build time, silently
# building "Debug" with Release flags/CRT. $<CONFIG:Debug> resolves correctly
# for both single- and multi-config generators. Quoted so the embedded `;`
# list separators survive CMake's argument parser instead of splitting the
# generator expression apart (cmake-generator-expressions(7)).
SET(STARFISH_CXXFLAGS_MODE
    "$<$<CONFIG:Debug>:/Od;/MDd>"
    "$<$<NOT:$<CONFIG:Debug>>:/O2;/MD>")

SET(STARFISH_DEFINES
        -DSTARFISH_VERSION_STR="${LWE_VERSION}"
        -DSTARFISH_WINDOWS
        -DSTARFISH_EXPORTS
        -D_TIMESPEC_DEFINED
        -D_USE_MATH_DEFINES
        -D_WINDOWS
        -D_USRDLL
        -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
        -D_CRT_SECURE_NO_WARNINGS
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

IF (STARFISH_WINDOWS_ENABLE_MULTIMEDIA)
    LIST (APPEND STARFISH_DEFINES -DSTARFISH_ENABLE_MULTIMEDIA)
ENDIF()

# Same generator-expression reasoning as STARFISH_CXXFLAGS_MODE above. Any
# non-Debug config (Release, RelWithDebInfo, ...) gets the release-type
# defines, matching the top-level CMakeLists.txt's accepted CMAKE_BUILD_TYPE
# values -- the old configure-time IF/ELSEIF/ELSE FATAL_ERROR'd on
# RelWithDebInfo even though the top-level check allows it.
#
# STARFISH_ENABLE_TEST (unlike on Linux/Tizen) is deliberately never defined
# here: it gates the LWERecord API-recorder/replayer and CompositorGL's GL
# debug-callback hook, both POSIX-only (sys/time.h's gettimeofday, dlfcn.h's
# dlsym/RTLD_DEFAULT) and never ported to Windows. Genuinely building a
# Windows Debug config for the first time (see the multi-config fix above)
# surfaced this as a hard C1083 (header not found) -- defining it here would
# require porting or platform-gating that POSIX-only code, which is out of
# scope for a build-system fix.
SET (STARFISH_DEFINES_MODE
    "$<$<CONFIG:Debug>:-DGC_DEBUG;-D_GLIBCXX_DEBUG>" # bdwgc
    "$<$<NOT:$<CONFIG:Debug>>:-DNDEBUG>")

SET (STARFISH_INCLUDE_DIRS
    ${STARFISH_ROOT}/inc
    ${STARFISH_ROOT}/src
    ${THIRD_PARTY_ROOT}/escargot/third_party/GCutil
    ${THIRD_PARTY_ROOT}/escargot/third_party/GCutil/include
    ${THIRD_PARTY_ROOT}/escargot/third_party/GCutil/include/gc
    ${THIRD_PARTY_ROOT}/escargot/src/api
    ${THIRD_PARTY_ROOT}/clipper/cpp
    ${THIRD_PARTY_ROOT}/skia_matrix
    ${THIRD_PARTY_ROOT}/skia_matrix/include/core
    ${THIRD_PARTY_ROOT}/skia_matrix/include/private
    ${THIRD_PARTY_ROOT}/earcut.hpp/include/mapbox
    ${THIRD_PARTY_ROOT}/MP4Parse/source/include
    ${THIRD_PARTY_ROOT}/escargot/third_party/windows/icu/include
    ${THIRD_PARTY_ROOT}/escargot/third_party/rapidjson/include
    ${THIRD_PARTY_ROOT}/rapidxml
    ${THIRD_PARTY_ROOT}/robin_map/include
    ${THIRD_PARTY_ROOT}/webm
    ${THIRD_PARTY_ROOT}/MP4Parse/source
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

# Multi-config generators (Visual Studio) append their own <Config>
# subdirectory to CMAKE_*_OUTPUT_DIRECTORY automatically, UNLESS the matching
# per-config _<CONFIG> variable is also set on it -- so setting a fixed
# .../Release here unconditionally used to make VS nest it a second time
# (.../Release/Release/), while pinning the _<CONFIG> variant to that same
# fixed value fixed the nesting but silently built every config (Debug
# included) with Release's own directory/flags, since STARFISH_CXXFLAGS_MODE/
# STARFISH_DEFINES_MODE were previously chosen from configure-time
# CMAKE_BUILD_TYPE too. Now that those are generator expressions (see above),
# leaving CMAKE_*_OUTPUT_DIRECTORY unsuffixed here lets CMake's own <Config>
# auto-append do the right thing per config (.../Debug/, .../Release/) for
# multi-config generators. Single-config generators (Ninja) never auto-append
# anything, so append CMAKE_BUILD_TYPE (guaranteed non-empty at this point --
# see the top-level CMakeLists.txt) ourselves to keep today's layout.
IF (STARFISH_IS_MULTI_CONFIG)
    SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
    SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
ELSE ()
    SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
    SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
    SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})
ENDIF ()

#######################################################
# ESCARGOT
#######################################################
IF (${ENABLE_WASM} STREQUAL "1")
    SET (ESCARGOT_WASM ON)
ENDIF()
IF (${ENABLE_CODECACHE} STREQUAL "1")
    SET (ESCARGOT_CODE_CACHE ON)
ENDIF()

IF (${ENABLE_DEBUGGER} STREQUAL "1")
    SET (ESCARGOT_DEBUGGER ON)
ENDIF()

SET (ESCARGOT_USE_CUSTOM_LOGGING ON)
SET (ESCARGOT_THREADING ON)
SET (ESCARGOT_LIBICU_SUPPORT ON)
SET (ESCARGOT_LIBICU_SUPPORT_WITH_DLOPEN ON)
SET (GCUTIL_WINDOWS_EXPORT_NORMALIZER
     ${STARFISH_ROOT}/build/windows_normalize_exports.cmake)

# /MP (MSVC parallel compilation) stays global and BEFORE add_subdirectory so
# it still reaches escargot's own targets too: it only affects build
# parallelism, never object code/behavior, so the target-scoping rule's
# purpose (avoid silently-changed behavior leaking across targets) doesn't
# apply to it.
add_compile_options("/MP")

SET (ESCARGOT_BUILD_SHARED_LIBS ON CACHE BOOL "Build Escargot as a Windows DLL" FORCE)
SET (ESCARGOT_BUILD_GC_SHARED_LIBS ON CACHE BOOL "Build GCutil as a shared library" FORCE)
SET (ESCARGOT_ENABLE_SHELL OFF CACHE BOOL "Do not build the Escargot shell for Starfish" FORCE)
ADD_SUBDIRECTORY (third_party/escargot)

SET_TARGET_PROPERTIES (escargot gc-lib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# ESCARGOT INTERNAL COMPILE OPTION -- only escargot's own sources read these
# (Escargot.h/VMInstance.cpp/ByteCode.cpp/ObjectStructure*), so scope them to
# the escargot target (post-hoc, after add_subdirectory creates it -- CMake
# resolves target_compile_definitions at generate time, so this still applies)
# instead of leaking into every target in this directory via global
# add_compile_options.
IF (TARGET escargot)
    target_compile_definitions (escargot PRIVATE)
ENDIF()

SET (STARFISH_DEPENDENCIES
    ${STARFISH_DEPENDENCIES}
    escargot
)

SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} escargot)

#######################################################
# JS BINDING
#######################################################
INCLUDE (${STARFISH_ROOT}/build/binding.cmake)
SET (STARFISH_INCLUDE_DIRS
    ${STARFISH_INCLUDE_DIRS}
    ${STARFISH_BINDING_INCLUDE_DIR})

#######################################################
# THIRD_PARTY (build outside)
#######################################################

INCLUDE (${STARFISH_ROOT}/build/windows_vcpkg.cmake)

#######################################################
# THIRD_PARTY (build here)
#######################################################
# ${STARFISH_CXXFLAGS_MODE} rather than a hardcoded /O2: it also carries
# /MD vs /MDd, so a Debug build would otherwise link these third-party targets
# against a different CRT than the engine.
SET (THIRD_PARTY_CXXFLAGS
 /std:c++14 ${STARFISH_CXXFLAGS_MODE} /Oy- /fp:strict /Zc:__cplusplus /EHs /source-charset:utf-8 /D_CRT_SECURE_NO_WARNINGS /DGC_NOT_DLL /D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING /wd4244 /wd4267 /wd4805 /wd4018 /wd4172
 ${STARFISH_CXXFLAGS_ARCH})
SET (THIRD_PARTY_CFLAGS
 ${STARFISH_CXXFLAGS_MODE} /Oy- /fp:strict /source-charset:utf-8 /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE /wd4244 /wd4267 /wd4018 /wd4101
 ${STARFISH_CXXFLAGS_ARCH})
SET (THIRD_PARTY_DEFINITIONS ${STARFISH_DEFINES_MODE})

#######################################################
# SKIA_MATRIX
#######################################################
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_CORE ${THIRD_PARTY_ROOT}/skia_matrix/src/core/*.cpp)
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_PORTS ${THIRD_PARTY_ROOT}/skia_matrix/src/ports/*.cpp)
ADD_LIBRARY (skia_matrix SHARED ${SKIA_MATRIX_SRC_CORE} ${SKIA_MATRIX_SRC_PORTS})
TARGET_INCLUDE_DIRECTORIES (skia_matrix PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix ${THIRD_PARTY_ROOT}/skia_matrix/include/core ${THIRD_PARTY_ROOT}/skia_matrix/include/private)
TARGET_COMPILE_DEFINITIONS (skia_matrix PRIVATE ${THIRD_PARTY_DEFINITIONS} SKIA_DLL SKIA_IMPLEMENTATION=1)
TARGET_COMPILE_OPTIONS (skia_matrix PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES (skia_matrix PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (skia_matrix PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} skia_matrix)

#######################################################
# LIBTUV
#######################################################
# The event-loop backend (PORT_EVENTLOOP_BACKEND_LIBUV). libtuv's own CMake
# graph is driven by its cross-build option/config files, so -- as with
# skia_matrix -- the Windows build file is kept here instead: the sources are
# exactly the ones libtuv's cmake/option/option_windows_common.cmake lists.
FILE (GLOB TUV_SRC_COMMON ${THIRD_PARTY_ROOT}/libtuv/src/*.c)
FILE (GLOB TUV_SRC_WIN ${THIRD_PARTY_ROOT}/libtuv/src/win/*.c)
ADD_LIBRARY (tuv SHARED ${TUV_SRC_COMMON} ${TUV_SRC_WIN})
TARGET_INCLUDE_DIRECTORIES (tuv PUBLIC ${THIRD_PARTY_ROOT}/libtuv/include ${THIRD_PARTY_ROOT}/libtuv/src)
# uv.h switches UV_EXTERN on these: dllexport while building the DLL,
# dllimport for everything that links it.
TARGET_COMPILE_DEFINITIONS (tuv
    PRIVATE ${THIRD_PARTY_DEFINITIONS} BUILDING_UV_SHARED _WINSOCK_DEPRECATED_NO_WARNINGS
    INTERFACE USING_UV_SHARED)
TARGET_COMPILE_OPTIONS (tuv PRIVATE ${THIRD_PARTY_CFLAGS})
TARGET_LINK_LIBRARIES (tuv PRIVATE ws2_32 psapi iphlpapi userenv advapi32 shell32 user32)
SET_TARGET_PROPERTIES (tuv PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (tuv PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} tuv)

#######################################################
# CLIPPER
#######################################################
FILE (GLOB CLIPPER_SRC ${THIRD_PARTY_ROOT}/clipper/cpp/*.cpp)
ADD_LIBRARY (clipper SHARED ${CLIPPER_SRC})
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PRIVATE ${THIRD_PARTY_DEFINITIONS} "CLIPPER_EXPORT=__declspec(dllexport)")
TARGET_COMPILE_OPTIONS (clipper PRIVATE ${THIRD_PARTY_CXXFLAGS})
SET_TARGET_PROPERTIES (clipper PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (clipper PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (clipper PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} clipper)

IF (STARFISH_WINDOWS_ENABLE_MULTIMEDIA)
    #######################################################
    # MP4PARSE
    #######################################################
    FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
    ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
    TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
    TARGET_COMPILE_DEFINITIONS (mp4parse PRIVATE ${THIRD_PARTY_DEFINITIONS})
    TARGET_COMPILE_OPTIONS (mp4parse PRIVATE ${THIRD_PARTY_CXXFLAGS})
    SET_TARGET_PROPERTIES (mp4parse PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} mp4parse)

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
    SET_TARGET_PROPERTIES (webm PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES ${STARFISH_THIRD_PARTY_LINK_LIBRARIES} webm)
ENDIF()



#######################################################
# STARFISH
#######################################################

FILE (GLOB_RECURSE STARFISH_SRC ${STARFISH_ROOT}/src/*.cpp)

FILE (GLOB_RECURSE STARFISH_SHELL_SRC ${STARFISH_ROOT}/src/shell/*.cpp)
FILE (GLOB_RECURSE STARFISH_CLI_SRC ${STARFISH_ROOT}/src/launcher/cli/*.cpp)
LIST (REMOVE_ITEM STARFISH_SRC
    ${STARFISH_ROOT}/src/launcher/CLI.cpp
    ${STARFISH_CLI_SRC}
    ${STARFISH_SHELL_SRC}
)

LIST (REMOVE_ITEM STARFISH_SRC ${STARFISH_ROOT}/src/platform/public/DeviceInfo.cpp)

FILE (GLOB STARFISH_SRC_GENRATED_BINDING ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/*.cpp)

SET (STARFISH_SRC_LIST
    ${STARFISH_SRC}
    ${STARFISH_SRC_GENRATED_BINDING}
)

SET (STARFISH_LINK_LIBRARIES icu.lib ${STARFISH_THIRD_PARTY_LINK_LIBRARIES})

ADD_LIBRARY (starfish.shared_library SHARED ${STARFISH_SRC_LIST})
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES OUTPUT_NAME "Starfish")
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
SET_TARGET_PROPERTIES (starfish.shared_library PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
TARGET_LINK_LIBRARIES (starfish.shared_library ${STARFISH_LINK_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES (starfish.shared_library PRIVATE ${STARFISH_INCLUDE_DIRS})
TARGET_COMPILE_OPTIONS (starfish.shared_library PRIVATE ${STARFISH_CXXFLAGS} ${STARFISH_CXXFLAGS_MODE} ${STARFISH_CXXFLAGS_ARCH})
TARGET_COMPILE_DEFINITIONS (starfish.shared_library PRIVATE ${STARFISH_DEFINES} ${STARFISH_DEFINES_MODE} SKIA_DLL SKIA_IMPLEMENTATION=0 "CLIPPER_EXPORT=__declspec(dllimport)")
ADD_DEPENDENCIES (starfish.shared_library ${STARFISH_DEPENDENCIES})

ADD_CUSTOM_COMMAND (TARGET starfish.shared_library POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${STARFISH_WINDOWS_VCPKG_RUNTIME_DIR}"
        "$<TARGET_FILE_DIR:starfish.shared_library>"
    COMMENT "Deploy vcpkg runtime DLLs"
)

IF (STARFISH_WINDOWS_BUILD_SHELL)
    FILE (GLOB STARFISH_WINDOWS_SHELL_SRC
        ${STARFISH_ROOT}/src/shell/windows/*.cpp)
    ADD_EXECUTABLE (starfish.windows_shell ${STARFISH_WINDOWS_SHELL_SRC})
    SET_TARGET_PROPERTIES (starfish.windows_shell PROPERTIES
        OUTPUT_NAME "StarfishShell"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    TARGET_INCLUDE_DIRECTORIES (starfish.windows_shell PRIVATE
        ${STARFISH_ROOT}/inc
        ${STARFISH_ROOT}/src/shell/windows)
    TARGET_COMPILE_OPTIONS (starfish.windows_shell PRIVATE
        ${STARFISH_CXXFLAGS}
        ${STARFISH_CXXFLAGS_MODE}
        ${STARFISH_CXXFLAGS_ARCH})
    # STARFISH_WINDOWS gates the shell sources themselves. STARFISH_EXPORTS is
    # deliberately NOT defined here: the shell is an embedder, so LWE_EXPORT
    # must resolve to dllimport against Starfish.dll.
    TARGET_COMPILE_DEFINITIONS (starfish.windows_shell PRIVATE
        STARFISH_WINDOWS _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
    # gdi32: ChoosePixelFormat/SetPixelFormat/SwapBuffers. opengl32: wgl* and
    # the OpenGL 1.1 entry points RendererWGL falls back to.
    TARGET_LINK_LIBRARIES (starfish.windows_shell PRIVATE
        starfish.shared_library user32 imm32 gdi32 opengl32)
    # The shell's entry point is wmain (wide argv, for URL/file arguments with
    # non-ASCII paths). CMake links the executable with an explicit
    # /subsystem:console, and link.exe then defaults to mainCRTStartup --
    # MSVCRT's exe_main.obj, which references a narrow main that does not
    # exist here (LNK2019). Name the wide startup object explicitly.
    TARGET_LINK_OPTIONS (starfish.windows_shell PRIVATE /ENTRY:wmainCRTStartup)
ENDIF()
