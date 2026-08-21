IF (NOT DEFINED INPUT_FILE)
    MESSAGE (FATAL_ERROR "INPUT_FILE is required")
ENDIF()

GET_FILENAME_COMPONENT (WINDOWS_EXPORT_INPUT_DIR "${INPUT_FILE}" DIRECTORY)
IF (NOT DEFINED BUILD_ROOT)
    GET_FILENAME_COMPONENT (BUILD_ROOT
        "${WINDOWS_EXPORT_INPUT_DIR}/../../../../../.." ABSOLUTE)
ENDIF()

# Scan the .obj files ourselves instead of trusting an externally-passed
# OBJECT_LIST: the intermediate file CMake's own WINDOWS_EXPORT_ALL_SYMBOLS
# machinery writes that list to (name, and whether it exists yet by the time
# this PRE_LINK command runs) differs across generators -- the Visual Studio
# generator schedules our PRE_LINK command ahead of its own def-generation
# step in the same PreLinkEvent, so that file isn't there yet. Object files
# themselves are guaranteed to exist by PRE_LINK time regardless of
# generator, so glob those directly.
FILE (GLOB WINDOWS_EXPORT_OBJECT_FILES "${WINDOWS_EXPORT_INPUT_DIR}/*.obj")
IF (NOT WINDOWS_EXPORT_OBJECT_FILES)
    # The caller (GCutil's CMakeLists.txt) always guesses the Ninja-style
    # ".../CMakeFiles/<target>.dir/<Config>/" layout for where
    # WINDOWS_EXPORT_ALL_SYMBOLS places exports.def and its object files.
    # The Visual Studio generator instead drops that "CMakeFiles/" path
    # component entirely (confirmed against a real MSBuild build log) -- fall
    # back to that layout if the primary guess found nothing.
    STRING (REPLACE "/CMakeFiles/" "/" WINDOWS_EXPORT_FALLBACK_DIR
            "${WINDOWS_EXPORT_INPUT_DIR}")
    IF (NOT WINDOWS_EXPORT_FALLBACK_DIR STREQUAL WINDOWS_EXPORT_INPUT_DIR)
        FILE (GLOB WINDOWS_EXPORT_OBJECT_FILES
              "${WINDOWS_EXPORT_FALLBACK_DIR}/*.obj")
        IF (WINDOWS_EXPORT_OBJECT_FILES)
            SET (WINDOWS_EXPORT_INPUT_DIR "${WINDOWS_EXPORT_FALLBACK_DIR}")
            GET_FILENAME_COMPONENT (WINDOWS_EXPORT_INPUT_NAME "${INPUT_FILE}" NAME)
            SET (INPUT_FILE "${WINDOWS_EXPORT_INPUT_DIR}/${WINDOWS_EXPORT_INPUT_NAME}")
        ENDIF()
    ENDIF()
ENDIF()
IF (NOT WINDOWS_EXPORT_OBJECT_FILES)
    MESSAGE (FATAL_ERROR "No object files found in ${WINDOWS_EXPORT_INPUT_DIR}")
ENDIF()

SET (WINDOWS_EXPORT_OBJECT_LIST "${INPUT_FILE}.objs")
FILE (WRITE "${WINDOWS_EXPORT_OBJECT_LIST}" "")
FOREACH (WINDOWS_EXPORT_OBJECT_FILE ${WINDOWS_EXPORT_OBJECT_FILES})
    FILE (APPEND "${WINDOWS_EXPORT_OBJECT_LIST}" "${WINDOWS_EXPORT_OBJECT_FILE}\n")
ENDFOREACH()

EXECUTE_PROCESS (
    COMMAND ${CMAKE_COMMAND} -E __create_def ${INPUT_FILE} ${WINDOWS_EXPORT_OBJECT_LIST}
    WORKING_DIRECTORY ${BUILD_ROOT}
    RESULT_VARIABLE WINDOWS_EXPORT_CREATE_RESULT)
IF (NOT WINDOWS_EXPORT_CREATE_RESULT EQUAL 0)
    MESSAGE (FATAL_ERROR "Failed to create ${INPUT_FILE}")
ENDIF()

FILE (STRINGS "${INPUT_FILE}" WINDOWS_EXPORT_LINES)
FILE (WRITE "${INPUT_FILE}" "EXPORTS\n")

FOREACH (WINDOWS_EXPORT_LINE ${WINDOWS_EXPORT_LINES})
    STRING (STRIP "${WINDOWS_EXPORT_LINE}" WINDOWS_EXPORT_ENTRY)
    IF (WINDOWS_EXPORT_ENTRY MATCHES "^_GC_.*")
        STRING (REGEX REPLACE "^_" "" WINDOWS_EXPORT_ENTRY
               "${WINDOWS_EXPORT_ENTRY}")
        FILE (APPEND "${INPUT_FILE}" "\t${WINDOWS_EXPORT_ENTRY}\n")
    ELSEIF (WINDOWS_EXPORT_ENTRY MATCHES "^GC_.*"
            OR WINDOWS_EXPORT_ENTRY MATCHES "^\\?.+")
        FILE (APPEND "${INPUT_FILE}" "\t${WINDOWS_EXPORT_ENTRY}\n")
    ENDIF()
ENDFOREACH()
