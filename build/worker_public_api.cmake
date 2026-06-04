CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET (STARFISH_WORKER_API_INCLUDE_DIRS ${STARFISH_API_INCLUDE_DIRS})

SET (STARFISH_WORKER_API_DEFINES -DSTARFISH_WEBWORKER_HOST)

IF (${MODE} STREQUAL "debug")
    SET (STARFISH_WORKER_API_DEFINES
        ${STARFISH_WORKER_API_DEFINES}
        -D_GLIBCXX_DEBUG
    )
ELSEIF (${MODE} STREQUAL "release")
    SET (STARFISH_WORKER_API_DEFINES
        ${STARFISH_WORKER_API_DEFINES} 
        -DNDEBUG
    )
ENDIF()

IF (${ENABLE_DYNAMIC_LOADER} STREQUAL "1")
    SET (STARFISH_WORKER_API_DEFINES
        ${STARFISH_WORKER_API_DEFINES}
        -DSTARFISH_API_ENABLE_LOADER
        -DSTARFISH_API_DEFAULT_PATH="${STARFISH_API_DEFAULT_PATH}"
        -DSTARFISH_API_UWE_MOUNT_PATH="${STARFISH_API_UWE_MOUNT_PATH}"
    )
ENDIF()


SET (STARFISH_WORKER_API_LDFLAGS ${LWE_LDFLAGS})
SET (STARFISH_WORKER_API_LINK_LIBRARIES ${STARFISH_API_LINK_LIBRARIES})

MACRO (add_worker_api_taget file_name variable_name)
    SET (STARFISH_${variable_name}_API_OBJECT_LIBRARY starfish_${file_name}_api_object_library)
    ADD_LIBRARY (${STARFISH_${variable_name}_API_OBJECT_LIBRARY} OBJECT
        ${STARFISH_ROOT}/src/public/LWEWorker.cpp
        ${STARFISH_ROOT}/src/public/LWELoaderUtils.cpp
        ${STARFISH_ROOT}/src/public/LWEWorkerDelegateLoader.cpp
    )

    TARGET_INCLUDE_DIRECTORIES (${STARFISH_${variable_name}_API_OBJECT_LIBRARY} PUBLIC ${STARFISH_WORKER_API_INCLUDE_DIRS})

    # FIXME:
    # This block came from config.cmake. please remove this and enable SOVERSION and VERSION properties.
    # The spec file will also need to be modified.
    IF (${HOST} STREQUAL "tizen")
        IF (${BACKEND} STREQUAL "glib_cairo_gl")
            SET (STARFISH_${variable_name}_API_LDFLAGS ${STARFISH_WORKER_API_LDFLAGS} -Wl,-soname,liblightweight-web-engine-${file_name}.so.1)
        ENDIF()
    ENDIF()

    IF (${ENABLE_DYNAMIC_LOADER} STREQUAL "1")
        MESSAGE (STATUS "${variable_name} ENABLE DYNAMIC_LOADER")
        ADD_LIBRARY (starfish_api.${file_name}.shared_library SHARED $<TARGET_OBJECTS:${STARFISH_${variable_name}_API_OBJECT_LIBRARY}>)
        ADD_DEPENDENCIES(starfish_api.${file_name}.shared_library starfish.${file_name}.shared_library)
    
        GET_TARGET_PROPERTY(STARFISH_${variable_name}_OUTPUT_NAME starfish.${file_name}.shared_library OUTPUT_NAME)
    
        SET (STARFISH_${variable_name}_API_DEFINES
            ${STARFISH_WORKER_API_DEFINES}
            -DSTARFISH_ENABLE_${variable_name}
            -DSTARFISH_${variable_name}_API_TARGET_NAME="lib${STARFISH_${variable_name}_OUTPUT_NAME}.so"
        )
        SET (STARFISH_WORKER_API_LINK_LIBRARIES ${STARFISH_WORKER_API_LINK_LIBRARIES} dl)
        
        TARGET_COMPILE_OPTIONS (${STARFISH_${variable_name}_API_OBJECT_LIBRARY} PUBLIC ${LWE_CXXFLAGS} ${STARFISH_${variable_name}_API_DEFINES})

        TARGET_LINK_LIBRARIES (starfish_api.${file_name}.shared_library ${STARFISH_WORKER_API_LINK_LIBRARIES} ${STARFISH_WORKER_API_LDFLAGS})

        SET_TARGET_PROPERTIES (starfish_api.${file_name}.shared_library PROPERTIES
            # SOVERSION 1
            OUTPUT_NAME ${TARGETNAME}-${file_name}
            # VERSION "1.0.0"
        )
    ELSE()
        ADD_LIBRARY (starfish_api.${file_name}.shared_library SHARED $<TARGET_OBJECTS:${STARFISH_${variable_name}_API_OBJECT_LIBRARY}>)
        ADD_LIBRARY (starfish_api.${file_name}.static_library STATIC $<TARGET_OBJECTS:${STARFISH_${variable_name}_API_OBJECT_LIBRARY}>)
        ADD_DEPENDENCIES(starfish_api.${file_name}.shared_library starfish.${file_name}.shared_library)
        ADD_DEPENDENCIES(starfish_api.${file_name}.static_library starfish.${file_name}.static_library)

        SET (STARFISH_${variable_name}_API_DEFINES 
            ${STARFISH_WORKER_API_DEFINES}
            -DSTARFISH_ENABLE_${variable_name}
            -DSTARFISH_WEBWORKER_HOST
        )
        TARGET_COMPILE_OPTIONS (${STARFISH_${variable_name}_API_OBJECT_LIBRARY} PUBLIC ${LWE_CXXFLAGS} ${STARFISH_${variable_name}_API_DEFINES})

        GET_TARGET_PROPERTY(STARFISH_${variable_name}_OUTPUT_NAME starfish.${file_name}.shared_library OUTPUT_NAME)
        SET (STARFISH_${variable_name}_API_LINK_LIBRARIES ${STARFISH_${variable_name}_OUTPUT_NAME})

        TARGET_LINK_LIBRARIES (starfish_api.${file_name}.shared_library 
            ${STARFISH_${variable_name}_API_LINK_LIBRARIES} ${STARFISH_${variable_name}_API_LDFLAGS})
        TARGET_LINK_LIBRARIES (starfish_api.${file_name}.static_library 
            ${STARFISH_${variable_name}_API_LINK_LIBRARIES} ${STARFISH_${variable_name}_API_LDFLAGS})

        SET_TARGET_PROPERTIES (starfish_api.${file_name}.shared_library PROPERTIES
            # SOVERSION 1
            OUTPUT_NAME ${TARGETNAME}-${file_name}
            # VERSION "1.0.0"
        )
        SET_TARGET_PROPERTIES (starfish_api.${file_name}.static_library PROPERTIES OUTPUT_NAME ${TARGETNAME}-${file_name})
    ENDIF()

    MESSAGE (STATUS "${variable_name} Public API")
    MESSAGE (STATUS "FLAGS: " "${LWE_CXXFLAGS}")
    MESSAGE (STATUS "LIBRARIES: " "${STARFISH_${variable_name}_API_LINK_LIBRARIES}")
    MESSAGE (STATUS "DEFINITIONS: " "${STARFISH_${variable_name}_API_DEFINES}")
    MESSAGE (STATUS "LDFLAGS: " "${STARFISH_${variable_name}_API_LDFLAGS}")
    MESSAGE (STATUS "INCLUDE_DIRS: " "${STARFISH_WORKER_API_INCLUDE_DIRS}")
    MESSAGE ("")
ENDMACRO()

IF (${SHARED_WORKER} STREQUAL "1")
    add_worker_api_taget (sharedworker SHARED_WORKER)
ENDIF()

IF (${SERVICE_WORKER} STREQUAL "1")
    add_worker_api_taget (serviceworker SERVICE_WORKER)
ENDIF()


