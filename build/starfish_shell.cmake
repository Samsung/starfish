CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

INCLUDE(${STARFISH_ROOT}/build/starfish_shell_defines.cmake)
# SET_STARFISH_SHELL_DEFINES() now correctly returns LWE_DEFINITIONS plus the
# -DSTARFISH_SHELL_* flag for ${SHELL} (see starfish_shell_defines.cmake) --
# don't re-append LWE_DEFINITIONS here too, or it ends up duplicated.
SET_STARFISH_SHELL_DEFINES()


SET (STARFISH_SHELL_INCLUDE_DIRS
    ${STARFISH_ROOT}/inc
    ${STARFISH_ROOT}/src/shell/
)

IF(${SHELL} STREQUAL "efl_headless")
    SET(STARFISH_SHELL_LIBRARIES ecore glib-2.0)
ELSEIF (${SHELL} STREQUAL "efl")
    SET(STARFISH_SHELL_LIBRARIES elementary evas ecore)
ELSEIF (${SHELL} STREQUAL "ecore_x")
    SET(STARFISH_SHELL_LIBRARIES ecore ecore-x ecore-input ecore-imf glib-2.0)
ELSEIF (${SHELL} STREQUAL "ecore_wl2")
    SET(STARFISH_SHELL_LIBRARIES ecore ecore-wl2 ecore-input ecore-imf glib-2.0 wayland-client)
ELSEIF (${SHELL} STREQUAL "tcore_wl")
    SET(STARFISH_SHELL_LIBRARIES tizen-core tizen-core-wl tizen-core-imf glib-2.0 wayland-client)
ELSEIF (${SHELL} STREQUAL "tcore_headless")
    SET(STARFISH_SHELL_LIBRARIES tizen-core)
ELSEIF (${SHELL} STREQUAL "glib_headless")
    SET(STARFISH_SHELL_LIBRARIES glib-2.0)
ELSEIF (${SHELL} STREQUAL "x11")
    SET(STARFISH_SHELL_LIBRARIES glib-2.0 x11 egl)
ELSEIF (${SHELL} STREQUAL "x11_webcontainer")
    SET(STARFISH_SHELL_LIBRARIES glib-2.0 x11 egl)
ENDIF()


SET(STARFISH_SHELL_LINK_LIBRARIES -lpthread)

IF(STARFISH_SHELL_LIBRARIES)
    PKG_CHECK_MODULES(STARFISH_SHELL_REQUIRED REQUIRED ${STARFISH_SHELL_LIBRARIES})

    SET (STARFISH_SHELL_INCLUDE_DIRS
        ${STARFISH_SHELL_INCLUDE_DIRS}
        ${STARFISH_SHELL_REQUIRED_INCLUDE_DIRS}
    )
    LIST(APPEND STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_REQUIRED_LIBRARIES})
ENDIF()

IF ("${STARFISH_DEPENDENCIES_EXTRA}" MATCHES "tuv")
    SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} -ltuv)
    SET (STARFISH_SHELL_INCLUDE_DIRS
        ${STARFISH_SHELL_INCLUDE_DIRS}
        ${TUV_BUILD_DIR}/include
    )
ENDIF()

SET(STARFISH_SHELL_DEPENDENCIES)

IF(${SHELL} STREQUAL "x11" )
    SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} GLESv2 EGL X11)
ELSEIF (${SHELL} STREQUAL "x11_webcontainer")
    SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} GLESv2 EGL X11)
ELSEIF (${SHELL} STREQUAL "ecore_x")
    SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} GLESv2 EGL)
ELSEIF (${SHELL} STREQUAL "ecore_wl2")
    SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} GLESv2 EGL)
ELSEIF (${SHELL} STREQUAL "tcore_wl")
    SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} GLESv2 EGL)
ENDIF()

SET(STARFISH_SHELL_LDFLAGS "")
IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET(STARFISH_SHELL_LDFLAGS -Wl,-rpath='\$\$ORIGIN/../lib')
ENDIF()

FILE (GLOB_RECURSE STARFISH_SHELL_SRC ${STARFISH_ROOT}/src/shell/*.cpp)

# backtrace
IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(ENABLE_BACKTRACE "TRUE")
    IF(${CMAKE_CXX_COMPILER} MATCHES "clang")
        EXECUTE_PROCESS(
            COMMAND bash -c "find /usr/lib/gcc /usr/local/include /usr/include -name backtrace.h | head -n 1"
            OUTPUT_VARIABLE BACKTRACE_H_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        IF(BACKTRACE_H_PATH)
            GET_FILENAME_COMPONENT(BACKTRACE_INC_DIR ${BACKTRACE_H_PATH} DIRECTORY)
            SET(STARFISH_SHELL_INCLUDE_DIRS ${STARFISH_SHELL_INCLUDE_DIRS} ${BACKTRACE_INC_DIR})
            MESSAGE(STATUS "Found libbacktrace header: ${BACKTRACE_INC_DIR}")

            EXECUTE_PROCESS(
                COMMAND bash -c "find $(dirname ${BACKTRACE_INC_DIR}) /usr/lib /usr/local/lib -name libbacktrace.a | head -n 1"
                OUTPUT_VARIABLE BACKTRACE_LIB_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            IF(BACKTRACE_LIB_PATH)
                GET_FILENAME_COMPONENT(BACKTRACE_LIB_DIR ${BACKTRACE_LIB_PATH} DIRECTORY)
                LINK_DIRECTORIES(${BACKTRACE_LIB_DIR})
                MESSAGE(STATUS "Found libbacktrace library: ${BACKTRACE_LIB_DIR}")
            ELSE()
                SET(ENABLE_BACKTRACE "FALSE")
            ENDIF()
        ELSE()
            SET(ENABLE_BACKTRACE "FALSE")
            MESSAGE(WARNING "libbacktrace (backtrace.h) not found. Build might fail.")
        ENDIF()
    ENDIF()
    IF(${ENABLE_BACKTRACE} STREQUAL "TRUE")
        MESSAGE(STATUS "ENABLE BACKTRACE")
        SET (STARFISH_SHELL_DEFINES ${STARFISH_SHELL_DEFINES} -DSTARFISH_SHELL_ENABLE_BACKTRACE)
        SET(STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} -lbacktrace)
    ENDIF()
ENDIF()

# gtest
SET (STARFISH_GTEST_DIR ${STARFISH_ROOT}/third_party/googletest/googletest)

IF (CMAKE_VERSION VERSION_LESS 3.5)
    # googletest's own CMakeLists requires CMake 3.5, which is newer than the
    # CMake shipped by some target build roots (Tizen 5.0 has 2.8.12). Build the
    # single-file amalgamation directly instead; gtest 1.11 only needs C++11.
    ADD_LIBRARY (gtest STATIC ${STARFISH_GTEST_DIR}/src/gtest-all.cc)
    # SYSTEM matches what googletest's own CMakeLists exports. The shell builds
    # with -Wextra -Werror and gtest's macros expand to code that trips
    # -Wsign-compare, so its headers must not be warned about in consumers.
    TARGET_INCLUDE_DIRECTORIES (gtest SYSTEM PUBLIC
        ${STARFISH_GTEST_DIR}/include
        ${STARFISH_GTEST_DIR}
    )
    # Same -fno-lto reason as the ADD_SUBDIRECTORY path below.
    SET_TARGET_PROPERTIES (gtest PROPERTIES COMPILE_FLAGS "-fno-lto")
    SET (STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} gtest -lpthread)
ELSE ()
    SET (BUILD_GMOCK OFF)
    SET (INSTALL_GTEST OFF)
    ADD_SUBDIRECTORY (third_party/googletest)
    # Build gtest without LTO. On Tizen the shell is force-linked with -fno-lto
    # (see LWE_*_FORCE_NOLTO in config.cmake), so LTO objects inside libgtest.a
    # cannot be consumed by ld at link time (gcc14/binutils: "plugin needed to
    # handle lto object") and produce undefined references. Scope -fno-lto to
    # the actual gtest targets (post-hoc, after add_subdirectory creates them --
    # CMake resolves target_compile_options at generate time, so this still
    # applies) instead of a global CMAKE_C_FLAGS/CMAKE_CXX_FLAGS save/restore
    # around add_subdirectory, which would affect every target googletest's own
    # CMakeLists happens to create, not just the ones actually linked in.
    FOREACH (GTEST_TARGET gtest gtest_main)
        IF (TARGET ${GTEST_TARGET})
            TARGET_COMPILE_OPTIONS (${GTEST_TARGET} PRIVATE -fno-lto)
        ENDIF()
    ENDFOREACH()
    SET (STARFISH_SHELL_LINK_LIBRARIES ${STARFISH_SHELL_LINK_LIBRARIES} gtest)
ENDIF ()

ADD_EXECUTABLE (starfish.executable ${STARFISH_SHELL_SRC})
ADD_DEPENDENCIES (starfish.executable starfish_api.shared_library ${STARFISH_SHELL_DEPENDENCIES})

STRING (REPLACE ";" " " STARFISH_SHELL_CXXFLAGS_PRINT "${LWE_CXXFLAGS}")
STRING (REPLACE ";" " " STARFISH_SHELL_LIBRARIES_PRINT "${STARFISH_SHELL_LINK_LIBRARIES} ${STARFISH_SHELL_LIBRARIES}")
STRING (REPLACE ";" " " STARFISH_SHELL_DEFINES_PRINT "${STARFISH_SHELL_DEFINES}")
STRING (REPLACE ";" " " STARFISH_SHELL_LDFLAGS_PRINT "${STARFISH_SHELL_LDFLAGS} ${LWE_LDFLAGS}")
STRING (REPLACE ";" " " STARFISH_SHELL_INCLUDE_DIRS_PRINT "${STARFISH_SHELL_INCLUDE_DIRS}")
MESSAGE (STATUS "Shell(${SHELL})")
MESSAGE (STATUS "FLAGS: ${STARFISH_SHELL_CXXFLAGS_PRINT}")
MESSAGE (STATUS "LIBRARIES: ${STARFISH_SHELL_LIBRARIES_PRINT}")
MESSAGE (STATUS "DEFINITIONS: ${STARFISH_SHELL_DEFINES_PRINT}")
MESSAGE (STATUS "LDFLAGS: ${STARFISH_SHELL_LDFLAGS_PRINT}")
MESSAGE (STATUS "INCLUDE_DIRS: ${STARFISH_SHELL_INCLUDE_DIRS_PRINT}")

TARGET_INCLUDE_DIRECTORIES (starfish.executable PUBLIC ${STARFISH_SHELL_INCLUDE_DIRS})
TARGET_COMPILE_OPTIONS (starfish.executable PUBLIC ${LWE_CXXFLAGS} ${STARFISH_SHELL_DEFINES})
TARGET_LINK_LIBRARIES (starfish.executable
    ${STARFISH_SHELL_LDFLAGS}
    ${LWE_LDFLAGS}
    ${TARGETNAME}
    ${STARFISH_SHELL_LINK_LIBRARIES}
    ${STARFISH_SHELL_REQUIRED_LINK_LIBRARIES}
)
SET_TARGET_PROPERTIES (starfish.executable PROPERTIES OUTPUT_NAME ${TARGETNAME})

IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    ADD_CUSTOM_COMMAND (TARGET starfish.executable POST_BUILD
        COMMAND ln -fs ${OUTPUT_DIRECTORY}/bin/${TARGETNAME} ${STARFISH_ROOT}/Starfish
    )
ENDIF()

MESSAGE(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "Compiler ID: ${CMAKE_C_COMPILER_ID}")
