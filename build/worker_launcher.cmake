CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET (STARFISH_WORKER_LAUNCHER_INCLUDE_DIRS
    ${STARFISH_ROOT}/inc
)

SET (STARFISH_SHARED_WORKER_ENTRY ${STARFISH_ROOT}/src/launcher/SharedWorkerEntry.cpp)
SET (STARFISH_SERVICE_WORKER_ENTRY ${STARFISH_ROOT}/src/launcher/ServiceWorkerEntry.cpp)

SET (STARFISH_WORKER_LAUNCHER_LDFLAGS "")
IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (STARFISH_WORKER_LAUNCHER_LDFLAGS -Wl,-rpath='\$\$ORIGIN/../lib')
ENDIF()

SET (STARFISH_WORKER_LAUNCHER_LINK_LIBRARIES -lpthread)

MACRO (add_worker_launcher file_name variable_name)

    ADD_EXECUTABLE (starfish.${file_name}.executable ${STARFISH_${variable_name}_ENTRY})
    ADD_DEPENDENCIES (starfish.${file_name}.executable starfish_api.${file_name}.shared_library)

    TARGET_INCLUDE_DIRECTORIES (starfish.${file_name}.executable PUBLIC ${STARFISH_WORKER_LAUNCHER_INCLUDE_DIRS})
    TARGET_COMPILE_DEFINITIONS (starfish.${file_name}.executable PUBLIC ${STARFISH_${variable_name}_DEFINITIONS})
    TARGET_COMPILE_OPTIONS (starfish.${file_name}.executable PUBLIC ${STARFISH_WORKER_CXXFLAGS})

    TARGET_LINK_LIBRARIES (starfish.${file_name}.executable PRIVATE 
        ${STARFISH_WORKER_LAUNCHER_LDFLAGS}
        ${STARFISH_WORKER_LDFLAGS}
        ${TARGETNAME}-${file_name}
        ${STARFISH_WORKER_LAUNCHER_LINK_LIBRARIES}
    )

    SET_TARGET_PROPERTIES (starfish.${file_name}.executable PROPERTIES OUTPUT_NAME ${TARGETNAME}-${file_name})

    IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        ADD_CUSTOM_COMMAND (TARGET starfish.${file_name}.executable POST_BUILD
            COMMAND ln -fs ${OUTPUT_DIRECTORY}/bin/${TARGETNAME}-${file_name} ${STARFISH_ROOT}/Starfish-${file_name}
        )
    ENDIF()

    MESSAGE (STATUS "Worker Launcher (${variable_name})")
    MESSAGE (STATUS "FLAGS: " "${STARFISH_WORKER_CXXFLAGS}")
    MESSAGE (STATUS "LIBRARIES: " "${STARFISH_WORKER_LAUNCHER_LINK_LIBRARIES}")
    MESSAGE (STATUS "DEFINITIONS: " "${STARFISH_${variable_name}_DEFINITIONS}")
    MESSAGE (STATUS "LDFLAGS: " "${STARFISH_WORKER_LAUNCHER_LDFLAGS} ${STARFISH_WORKER_LDFLAGS}")
    MESSAGE (STATUS "INCLUDE_DIRS: " "${STARFISH_WORKER_LAUNCHER_INCLUDE_DIRS}")
    MESSAGE ("")

ENDMACRO()

IF (${SHARED_WORKER} STREQUAL "1")
    add_worker_launcher (sharedworker SHARED_WORKER)
ENDIF()

IF (${SERVICE_WORKER} STREQUAL "1")
    add_worker_launcher (serviceworker SERVICE_WORKER)
ENDIF()
