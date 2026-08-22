IF (NOT PYTHON_EXECUTABLE)
    IF (CMAKE_VERSION VERSION_LESS "3.12")
        # Unversioned FIND_PACKAGE(PythonInterp) tries a bare "python" name
        # before any python3.x name -- on a system with a legacy python2
        # symlinked to plain "python" (some older distros), that resolves to
        # Python 2, not 3. Passing the major version restricts the name list
        # to "python3"/"python3.x", sidestepping the bare name entirely.
        FIND_PACKAGE (PythonInterp 3 REQUIRED)
    ELSE()
        FIND_PACKAGE (Python3 COMPONENTS Interpreter REQUIRED)
        SET (PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    ENDIF()
ENDIF()

#######################################################
# GENERATE BINDING
#######################################################

SET (STARFISH_BINDING_GENERATED_DIR ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated)
SET (STARFISH_BINDING_INCLUDE_DIR ${OUTPUT_DIRECTORY}/starfish_generated/)
SET (STARFISH_BINDING_STAMP ${OUTPUT_DIRECTORY}/starfish_generated/binding/binding_inputs.stamp)

# Collect every input the generator depends on: the generator scripts and
# templates plus all IDL files. Adding, modifying or deleting any of these must
# trigger regeneration; nothing else should. The glob and the mtime signature
# below are evaluated at configure time only (no CONFIGURE_DEPENDS), so any
# .idl change requires re-running cmake -- an incremental ninja won't see it.
FILE (GLOB_RECURSE STARFISH_BINDING_IDL_FILES ${STARFISH_ROOT}/src/*.idl)
FILE (GLOB STARFISH_BINDING_GENERATOR_FILES
    ${STARFISH_ROOT}/binding_generator/scripts/*.py
    ${STARFISH_ROOT}/binding_generator/scripts/templates/*
)
SET (STARFISH_BINDING_INPUTS ${STARFISH_BINDING_GENERATOR_FILES} ${STARFISH_BINDING_IDL_FILES})
LIST (SORT STARFISH_BINDING_INPUTS)

# Build a signature from each input's path and last-modified time. Sorting the
# list first means additions and deletions change the signature too, so the
# hash captures add / modify / delete of any input.
SET (STARFISH_BINDING_SIGNATURE "")
FOREACH (STARFISH_BINDING_INPUT ${STARFISH_BINDING_INPUTS})
    FILE (TIMESTAMP ${STARFISH_BINDING_INPUT} STARFISH_BINDING_INPUT_MTIME UTC)
    SET (STARFISH_BINDING_SIGNATURE "${STARFISH_BINDING_SIGNATURE}${STARFISH_BINDING_INPUT}|${STARFISH_BINDING_INPUT_MTIME}\n")
ENDFOREACH()
STRING (MD5 STARFISH_BINDING_SIGNATURE_HASH "${STARFISH_BINDING_SIGNATURE}")

# Regenerate only when a previous result is missing or the input signature
# changed. Otherwise the existing generated code is already up to date.
SET (STARFISH_BINDING_NEED_GENERATE TRUE)
IF (EXISTS ${STARFISH_BINDING_GENERATED_DIR}/Interfaces.h AND EXISTS ${STARFISH_BINDING_STAMP})
    FILE (READ ${STARFISH_BINDING_STAMP} STARFISH_BINDING_PREV_HASH)
    IF (STARFISH_BINDING_PREV_HASH STREQUAL STARFISH_BINDING_SIGNATURE_HASH)
        SET (STARFISH_BINDING_NEED_GENERATE FALSE)
    ENDIF()
ENDIF()

IF (STARFISH_BINDING_NEED_GENERATE)
    MESSAGE (STATUS "GENERATE BINDING: inputs changed, regenerating binding code")

    # Generate binding code into a scratch directory first. Each step must run in
    # order: EXECUTE_PROCESS treats multiple COMMANDs as a pipeline and starts
    # them concurrently, so the directory setup has to happen separately from the
    # generator invocation.
    FILE (REMOVE_RECURSE ${OUTPUT_DIRECTORY}/starfish_generated/binding_test)
    FILE (MAKE_DIRECTORY ${STARFISH_BINDING_GENERATED_DIR})
    FILE (MAKE_DIRECTORY ${OUTPUT_DIRECTORY}/starfish_generated/binding_test/generated)

    EXECUTE_PROCESS(
        COMMAND ${PYTHON_EXECUTABLE} ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${OUTPUT_DIRECTORY}/starfish_generated/binding_test/generated/
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error_output
    )

    IF (NOT _result EQUAL 0)
        MESSAGE(STATUS "Output:\n${_output}")
        MESSAGE(FATAL_ERROR "${_error_output}")
    ENDIF()

    # Copy only the binding files whose content actually changed so that
    # untouched files keep their timestamps and avoid needless recompiles.
    FILE (GLOB STARFISH_BINDING_TEST_FILES ${OUTPUT_DIRECTORY}/starfish_generated/binding_test/generated/*)
    FOREACH (STARFISH_BINDING_TEST_FILE ${STARFISH_BINDING_TEST_FILES})
        GET_FILENAME_COMPONENT (STARFISH_BINDING_FILE ${STARFISH_BINDING_TEST_FILE} NAME)
        EXECUTE_PROCESS (COMMAND ${CMAKE_COMMAND} -E compare_files ${STARFISH_BINDING_TEST_FILE} ${STARFISH_BINDING_GENERATED_DIR}/${STARFISH_BINDING_FILE}
                        RESULT_VARIABLE BINDING_COMPARE_RESULT
        )

        IF (${BINDING_COMPARE_RESULT} EQUAL 0)
            # leave below line for debugging cmake file
            # MESSAGE (STATUS ${STARFISH_BINDING_TEST_FILE} ${STARFISH_BINDING_GENERATED_DIR}/${STARFISH_BINDING_FILE} " are same")
        ELSE()
            # The files are different or error while comparing the files.
            FILE (COPY ${STARFISH_BINDING_TEST_FILE} DESTINATION ${STARFISH_BINDING_GENERATED_DIR})
        ENDIF()
    ENDFOREACH()

    # Remove stale generated files whose source IDL was deleted.
    FILE (GLOB STARFISH_BINDING_EXISTING_FILES ${STARFISH_BINDING_GENERATED_DIR}/*)
    FOREACH (STARFISH_BINDING_EXISTING_FILE ${STARFISH_BINDING_EXISTING_FILES})
        GET_FILENAME_COMPONENT (STARFISH_BINDING_FILE ${STARFISH_BINDING_EXISTING_FILE} NAME)
        IF (NOT EXISTS ${OUTPUT_DIRECTORY}/starfish_generated/binding_test/generated/${STARFISH_BINDING_FILE})
            FILE (REMOVE ${STARFISH_BINDING_EXISTING_FILE})
        ENDIF()
    ENDFOREACH()

    FILE (REMOVE_RECURSE ${OUTPUT_DIRECTORY}/starfish_generated/binding_test)

    # Record the signature so the next configure can skip regeneration.
    FILE (WRITE ${STARFISH_BINDING_STAMP} "${STARFISH_BINDING_SIGNATURE_HASH}")
ELSE()
    MESSAGE (STATUS "GENERATE BINDING: inputs unchanged, skipping binding generation")
ENDIF()

ADD_CUSTOM_TARGET (generate_binding
                   DEPENDS ${OUTPUT_DIRECTORY}/starfish_generated/binding/generated/Interfaces.h
                   COMMENT "GENERATE BINDING"
)
