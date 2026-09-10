IF (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
    NOT CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    MESSAGE (FATAL_ERROR "CLI supports Linux and Tizen only")
ENDIF ()
IF (NOT TARGET starfish.executable)
    MESSAGE (FATAL_ERROR "CLI requires a shell executable")
ENDIF ()

# The CLI implementation. The executable and the unit test both link it
SET (_cli_sources
    ${STARFISH_ROOT}/src/launcher/cli/CDPClient.cpp
    ${STARFISH_ROOT}/src/launcher/cli/ClientMain.cpp
    ${STARFISH_ROOT}/src/launcher/cli/DaemonMain.cpp
    ${STARFISH_ROOT}/src/launcher/cli/Protocol.cpp
    ${STARFISH_ROOT}/src/launcher/cli/Session.cpp
    ${STARFISH_ROOT}/src/launcher/cli/Snapshot.cpp
    ${STARFISH_ROOT}/src/launcher/cli/SocketPath.cpp
)
ADD_LIBRARY (starfish.cli STATIC ${_cli_sources})
UNSET (_cli_sources)

# Consumers include the CLI headers by name, so this directory is PUBLIC.
TARGET_INCLUDE_DIRECTORIES (starfish.cli PUBLIC
    ${STARFISH_ROOT}/src/launcher/cli
)
# rapidjson appears in the .cpp files only, never in a CLI header.
TARGET_INCLUDE_DIRECTORIES (starfish.cli PRIVATE
    ${ESCARGOT_THIRD_PARTY_ROOT}/rapidjson/include
)
# Constants.h reads these macros, so anything including that header needs them.
TARGET_COMPILE_DEFINITIONS (starfish.cli PUBLIC
    LWE_CLI_PROGRAM_NAME="${TARGETNAME}-cli"
    LWE_CLI_ENGINE_BINARY_NAME="${TARGETNAME}"
)
TARGET_COMPILE_OPTIONS (starfish.cli PRIVATE ${LWE_CXXFLAGS})

ADD_EXECUTABLE (starfish.cli.executable ${STARFISH_ROOT}/src/launcher/CLI.cpp)
TARGET_LINK_LIBRARIES (starfish.cli.executable starfish.cli)
TARGET_COMPILE_OPTIONS (starfish.cli.executable PRIVATE ${LWE_CXXFLAGS})
# The CLI starts the engine as a child process, so that binary must exist.
ADD_DEPENDENCIES (starfish.cli.executable starfish.executable)
SET_TARGET_PROPERTIES (starfish.cli.executable PROPERTIES
    OUTPUT_NAME "${TARGETNAME}-cli"
)

SET (_cli_unit_test_sources
    ${STARFISH_ROOT}/src/launcher/cli/test/ProtocolTest.cpp
    ${STARFISH_ROOT}/src/launcher/cli/test/SnapshotTest.cpp
    ${STARFISH_ROOT}/src/launcher/cli/test/TestMain.cpp
)
ADD_EXECUTABLE (starfish.cli.unit_test EXCLUDE_FROM_ALL
    ${_cli_unit_test_sources}
)
UNSET (_cli_unit_test_sources)
TARGET_LINK_LIBRARIES (starfish.cli.unit_test starfish.cli gtest -lpthread)
TARGET_COMPILE_OPTIONS (starfish.cli.unit_test PRIVATE ${LWE_CXXFLAGS})
SET_TARGET_PROPERTIES (starfish.cli.unit_test PROPERTIES
    OUTPUT_NAME "${TARGETNAME}-cli-unit-test"
)

MESSAGE (STATUS "LWE CLI enabled: ${TARGETNAME}-cli")
