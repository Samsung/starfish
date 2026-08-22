# The vcpkg toolchain file must be selected while configuring, before CMake's
# project() call. The Windows MSVC-on-Wine presets use this path for Intel x86
# and x64 builds.
# Remove the obsolete pthreads-win32 2.x runtime left by incremental builds.
# Fresh vcpkg builds deploy pthreadVC3.dll from the official source-built port.
FILE (REMOVE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pthreadVC2.dll")

IF (CMAKE_VERSION VERSION_LESS "3.18")
    MESSAGE (FATAL_ERROR "The Windows vcpkg dependency graph requires CMake 3.18 or newer")
ENDIF()

IF (NOT DEFINED VCPKG_TARGET_TRIPLET)
    IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
        SET (VCPKG_TARGET_TRIPLET "x86-windows" CACHE STRING "vcpkg target triplet")
    ELSEIF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x64")
        SET (VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
    ELSE()
        MESSAGE (FATAL_ERROR "vcpkg Windows dependencies support only Intel x86 and x86_64")
    ENDIF()
ENDIF()

IF (VCPKG_TARGET_TRIPLET MATCHES "-static")
    MESSAGE (FATAL_ERROR "Starfish Windows packages require the dynamic ${VCPKG_TARGET_TRIPLET} triplet so runtime DLLs can be deployed")
ENDIF()

# These package targets carry both their include paths and transitive link
# dependencies. Curl's ssl feature selects Windows SSPI/Schannel, and (as of
# libwebsockets 5.0.0, see vcpkg/ports-public/libwebsockets) so does
# libwebsockets itself -- neither pulls in OpenSSL on Windows any more.
# Starfish's own Windows hashing uses CNG.
FIND_PACKAGE (CURL CONFIG REQUIRED)
FIND_PACKAGE (Fontconfig REQUIRED)
FIND_PACKAGE (Freetype CONFIG REQUIRED)
FIND_PACKAGE (harfbuzz CONFIG REQUIRED)
FIND_PACKAGE (PNG CONFIG REQUIRED)
FIND_PACKAGE (GIF REQUIRED)
FIND_PACKAGE (GLEW CONFIG REQUIRED)
FIND_PACKAGE (libwebsockets CONFIG REQUIRED)
FIND_PACKAGE (PThreads4W CONFIG REQUIRED)

# cairo currently exports pkg-config metadata but no CMake package from this
# vcpkg baseline. Keep the workaround target-local so the rest of the graph
# still uses imported CMake targets.
FIND_PATH (STARFISH_VCPKG_CAIRO_INCLUDE_DIR cairo.h PATH_SUFFIXES cairo REQUIRED)
FIND_LIBRARY (STARFISH_VCPKG_CAIRO_LIBRARY NAMES cairo REQUIRED)
ADD_LIBRARY (starfish.vcpkg.cairo INTERFACE)
TARGET_INCLUDE_DIRECTORIES (starfish.vcpkg.cairo INTERFACE ${STARFISH_VCPKG_CAIRO_INCLUDE_DIR})
TARGET_LINK_LIBRARIES (starfish.vcpkg.cairo INTERFACE ${STARFISH_VCPKG_CAIRO_LIBRARY})

IF (TARGET websockets_shared)
    SET (STARFISH_VCPKG_LIBWEBSOCKETS_TARGET websockets_shared)
ELSEIF (TARGET websockets)
    SET (STARFISH_VCPKG_LIBWEBSOCKETS_TARGET websockets)
ELSEIF (TARGET libwebsockets::websockets)
    SET (STARFISH_VCPKG_LIBWEBSOCKETS_TARGET libwebsockets::websockets)
ELSE()
    MESSAGE (FATAL_ERROR "vcpkg libwebsockets did not export a supported CMake target")
ENDIF()

ADD_LIBRARY (starfish.windows.vcpkg INTERFACE)
TARGET_LINK_LIBRARIES (starfish.windows.vcpkg INTERFACE
    CURL::libcurl
    starfish.vcpkg.cairo
    Fontconfig::Fontconfig
    Freetype::Freetype
    harfbuzz::harfbuzz
    PNG::PNG
    GIF::GIF
    GLEW::GLEW
    ${STARFISH_VCPKG_LIBWEBSOCKETS_TARGET}
    PThreads4W::PThreads4W
)

# This is read only inside a POST_BUILD COMMAND (windows.cmake's
# copy_directory deploy step for starfish.shared_library), which CMake
# evaluates per-build, so a $<CONFIG:Debug> generator expression here
# resolves against the actual selected config -- unlike a configure-time
# IF(CMAKE_BUILD_TYPE STREQUAL "Debug"), which would always take the ELSE
# branch on a multi-config generator (CMAKE_BUILD_TYPE is empty at configure
# time there) regardless of --config, deploying Release vcpkg DLLs next to a
# Debug build.
SET (STARFISH_WINDOWS_VCPKG_RUNTIME_DIR
    "$<IF:$<CONFIG:Debug>,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin,${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin>")

SET (STARFISH_WINDOWS_VCPKG_FONTCONFIG_DIR
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/etc/fonts")

# Flattens the vcpkg-installed fonts.conf + conf.d/*.conf into one embeddable
# XML string (see the generator's own comment for why) so LWEDelegate.cpp can
# load Fontconfig via FcConfigParseAndLoadFromMemory() -- no fontconfig data
# files need to be deployed next to Starfish.dll or located at runtime.
SET (STARFISH_WINDOWS_FONTCONFIG_CONFIG_HEADER
    "${OUTPUT_DIRECTORY}/starfish_generated/WindowsFontconfigConfig.h")
FILE (MAKE_DIRECTORY "${OUTPUT_DIRECTORY}/starfish_generated")
EXECUTE_PROCESS (
    COMMAND ${PYTHON_EXECUTABLE}
        "${STARFISH_ROOT}/tool/build/gen_windows_fontconfig_config.py"
        "--fontconfig-dir" "${STARFISH_WINDOWS_VCPKG_FONTCONFIG_DIR}"
        "-o" "${STARFISH_WINDOWS_FONTCONFIG_CONFIG_HEADER}"
    RESULT_VARIABLE STARFISH_WINDOWS_FONTCONFIG_CONFIG_GEN_RESULT
)
IF (NOT STARFISH_WINDOWS_FONTCONFIG_CONFIG_GEN_RESULT EQUAL 0)
    MESSAGE (FATAL_ERROR "Failed to generate WindowsFontconfigConfig.h from ${STARFISH_WINDOWS_VCPKG_FONTCONFIG_DIR}")
ENDIF()

SET (STARFISH_THIRD_PARTY_LINK_LIBRARIES
    ${STARFISH_THIRD_PARTY_LINK_LIBRARIES}
    starfish.windows.vcpkg
)
