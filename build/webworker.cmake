CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# GUARD
IF (NOT ${HOST} STREQUAL "linux")
    RETURN()
ENDIF()

#######################################################
# CONFIG
#######################################################

SET (STARFISH_WEBWORKER_DEFINITIONS
    ${LWE_DEFINES_DEFAULT}
    -DGC_DEBUG # bdwgc
    -D_GLIBCXX_DEBUG
    -DSTARFISH_WEBWORKER_HOST
    -DSTARFISH_ENABLE_SERVICE_WORKER
    -DPORT_EVENTLOOP_BACKEND_LIBUV
    -DPORT_NEEDS_THREADED_PUBLIC_API
)

SET (STARFISH_WEBWORKER_LIBRARIES_DEFAULT pthread curl ssl crypto)
SET (STARFISH_WEBWORKER_DEPENDENCIES escargot gc tuv nanomsg)
SET (STARFISH_WEBWORKER_LIBRARIES_THIRD_PARTY escargot ${GC_TARGET} ${TUV_TARGET} ${NANOMSG_TARGET})
SET (STARFISH_WEBWORKER_INCLUDE_ADDITIONAL_DIRS 
    ${GCUTIL_ROOT} 
    ${GCUTIL_ROOT}/bdwgc/include
    ${ESCARGOT_ROOT}/src/api
    ${ESCARGOT_ROOT}/third_party/rapidjson/include
    ${THIRD_PARTY_ROOT}/nanomsg/dist/include
    ${THIRD_PARTY_ROOT}/nanomsgcpp)
SET (STARFISH_WEBWORKER_ENTRY ${STARFISH_ROOT}/src/launcher/WebWorkerEntry.cpp)

#######################################################
# SOURCE FILES
#######################################################

# Extract the path of the interface exposed to the Worker
FILE (GLOB_RECURSE STARFISH_IDL ${STARFISH_ROOT}/src/*.idl)
SET (STARFISH_WEBWORKER_EXPOSED_INTERFACE_SRC)
# TODO: include this interface or completely exclude in Worker.
SET (EXCLUDE_INTERFACE_NAME 
    "Navigator" "EventSource" "DOMStringList" "FormData"  "CSS")
FOREACH (IDL_FILE ${STARFISH_IDL})
    FILE (READ ${IDL_FILE} IDL_STRING)
    STRING (REGEX MATCH "[[].*Exposed=(Worker|.*,Worker)" MATCHED_IDL_FILE ${IDL_STRING})
    if (MATCHED_IDL_FILE)
        STRING (REGEX MATCH "[a-zA-Z0-9]+[.]idl" MATCHED_INTERFACE_NAME ${IDL_FILE})
        IF (MATCHED_INTERFACE_NAME)
            STRING(REPLACE ".idl" "" MATCHED_INTERFACE_NAME ${MATCHED_INTERFACE_NAME})
            LIST (FIND EXCLUDE_INTERFACE_NAME ${MATCHED_INTERFACE_NAME} MATCH_IDX)
            IF (${MATCH_IDX} LESS 0)
                # Add binding source file
                STRING(REPLACE ".idl" ".cpp" SOURCE_FILE ${IDL_FILE})
                IF (EXISTS ${SOURCE_FILE})
                    LIST (APPEND STARFISH_WEBWORKER_EXPOSED_INTERFACE_SRC ${SOURCE_FILE})
                ENDIF()
                #Add source file
                SET (INTERFACE_BINDING_SRC ${STARFISH_ROOT}/src/binding/${MATCHED_INTERFACE_NAME}Binding.cpp)
                IF (EXISTS ${INTERFACE_BINDING_SRC})
                    LIST (APPEND STARFISH_WEBWORKER_EXPOSED_INTERFACE_SRC ${INTERFACE_BINDING_SRC})
                ENDIF()
            ENDIF()
        ENDIF()
    ENDIF()
ENDFOREACH()

FILE (GLOB STARFISH_WEBWORKER_DEFAULT_SRC
    ${STARFISH_ROOT}/src/public/LWE.cpp
    ${STARFISH_ROOT}/src/StaticStrings.cpp
    ${STARFISH_ROOT}/src/Starfish.cpp
    ${STARFISH_ROOT}/src/platform/loader/ResourceURL.cpp
    ${STARFISH_ROOT}/src/platform/message_loop/*.cpp
    ${STARFISH_ROOT}/src/platform/network/curl/*.cpp
    ${STARFISH_ROOT}/src/platform/network/http/*.cpp
    ${STARFISH_ROOT}/src/platform/file/File.cpp
)

FILE (GLOB STARFISH_WEBWORKER_CORE_SRC 
    ${STARFISH_ROOT}/src/core/util/*.cpp
    ${STARFISH_ROOT}/src/core/fileapi/*.cpp
    ${STARFISH_ROOT}/src/core/extra/Console.cpp
    ${STARFISH_ROOT}/src/core/extra/MimeType.cpp
    ${STARFISH_ROOT}/src/core/page/WebBase.cpp
    ${STARFISH_ROOT}/src/core/page/NavigatorMixin.cpp
    ${STARFISH_ROOT}/src/core/page/Serializer.cpp
    ${STARFISH_ROOT}/src/core/modules/threading/*.cpp
    ${STARFISH_ROOT}/src/core/modules/resource_request/*.cpp
    ${STARFISH_ROOT}/src/core/modules/networking/*.cpp 
    ${STARFISH_ROOT}/src/core/modules/worker/host/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/host/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/client/*.cpp # TODO: remove client source files
    ${STARFISH_ROOT}/src/core/modules/profiling/Profiling.cpp
    ${STARFISH_ROOT}/src/core/dom/ExecutionContext.cpp
    ${STARFISH_ROOT}/src/core/dom/WebOrigin.cpp
    ${STARFISH_ROOT}/src/core/csp/*.cpp
    ${STARFISH_ROOT}/src/core/fetch/*.cpp
    ${STARFISH_ROOT}/src/core/fetch/stream/*.cpp
)

FILE (GLOB STARFISH_WEBWORKER_BINDING_SRC 
    ${STARFISH_ROOT}/src/binding/ScriptWrappable.cpp
    ${STARFISH_ROOT}/src/binding/ScriptEngineInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingWorkerInstance.cpp
    ${STARFISH_ROOT}/src/binding/RequestInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/BlobOrBufferSourceOrUSVStringOrReadableStreamBinding.cpp
    ${STARFISH_ROOT}/src/binding/ArrayBufferViewOrArrayBufferBinding.cpp
    ${STARFISH_ROOT}/src/binding/SecurityPolicyViolationEventInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/ResponseInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/ErrorEventInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/DOMPointInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/BlobCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/CustomEventInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/EventInitBinding.cpp
    ${STARFISH_ROOT}/src/binding/RequestOrUSVStringBinding.cpp
    ${STARFISH_ROOT}/src/binding/RegistrationOptionsBinding.cpp
    ${STARFISH_ROOT}/src/binding/WindowOrServiceWorkerBinding.cpp
    ${STARFISH_ROOT}/src/binding/MessageEventInitBinding.cpp
)

SET (STARFISH_WEBWORKER_SRC_LIST
    ${STARFISH_WEBWORKER_DEFAULT_SRC}
    ${STARFISH_WEBWORKER_CORE_SRC}
    ${STARFISH_WEBWORKER_BINDING_SRC}
    ${STARFISH_WEBWORKER_EXPOSED_INTERFACE_SRC}
)

#######################################################
# INCLUDE DIRS
#######################################################
SET (STARFISH_WEBWORKER_INCLUDE_DIRS
    ${STARFISH_ROOT}/src
    ${STARFISH_ROOT}/inc
    ${STARFISH_WEBWORKER_INCLUDE_ADDITIONAL_DIRS}
)

#######################################################
# LINK LIBRARIES
#######################################################
SET (STARFISH_WEBWORKER_LINK_LIBRARIES
    ${STARFISH_WEBWORKER_LIBRARIES_THIRD_PARTY}
    ${STARFISH_WEBWORKER_LIBRARIES_DEFAULT}
    ${STARFISH_LIBRARIES_COMPILER}
)

#######################################################
# BUILD TARGET
#######################################################
SET (STARFISH_WEBWORKER_OBJECT_LIBRARY starfish_webworker_object_library)
SET (STARFISH_WEBWORKER_OUTPUT_NAME ${TARGETNAME}WebWorker)

ADD_LIBRARY (${STARFISH_WEBWORKER_OBJECT_LIBRARY} OBJECT ${STARFISH_WEBWORKER_SRC_LIST})

ADD_EXECUTABLE (starfish.webworker 
                $<TARGET_OBJECTS:${STARFISH_WEBWORKER_OBJECT_LIBRARY}> 
                ${STARFISH_WEBWORKER_ENTRY})

# Create JavaScript binding source for worker
ADD_CUSTOM_TARGET (CREATE_JSBINDINGSOURCE
    COMMAND python ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${STARFISH_ROOT}/src/binding --exposed Worker
)
ADD_DEPENDENCIES (${STARFISH_WEBWORKER_OBJECT_LIBRARY} ${STARFISH_WEBWORKER_DEPENDENCIES} CREATE_JSBINDINGSOURCE)

ADD_CUSTOM_COMMAND (TARGET starfish.webworker POST_BUILD
    COMMAND ln -fs ${OUTPUT_DIRECTORY}/bin/${STARFISH_WEBWORKER_OUTPUT_NAME} ${STARFISH_ROOT}/${STARFISH_WEBWORKER_OUTPUT_NAME}
)


message (STATUS "FLAGS: " ${LWE_CXXFLAGS})
message (STATUS "LIBRARIES: " ${STARFISH_WEBWORKER_LINK_LIBRARIES})
message (STATUS "DEFINITIONS: " ${STARFISH_WEBWORKER_DEFINITIONS})
message (STATUS "LDFLAGS: " ${LWE_LDFLAGS})
message (STATUS "INCLUDE_DIRS: " ${STARFISH_WEBWORKER_INCLUDE_DIRS})

TARGET_INCLUDE_DIRECTORIES (${STARFISH_WEBWORKER_OBJECT_LIBRARY} PUBLIC ${STARFISH_WEBWORKER_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS (${STARFISH_WEBWORKER_OBJECT_LIBRARY} PUBLIC ${STARFISH_WEBWORKER_DEFINITIONS})
TARGET_COMPILE_OPTIONS (${STARFISH_WEBWORKER_OBJECT_LIBRARY} PUBLIC ${LWE_CXXFLAGS})

TARGET_INCLUDE_DIRECTORIES (starfish.webworker PUBLIC ${STARFISH_WEBWORKER_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS (starfish.webworker PUBLIC ${STARFISH_WEBWORKER_DEFINITIONS})
TARGET_COMPILE_OPTIONS (starfish.webworker PUBLIC ${LWE_CXXFLAGS})

TARGET_LINK_LIBRARIES (starfish.webworker ${STARFISH_WEBWORKER_LINK_LIBRARIES} ${LWE_LDFLAGS})
SET_TARGET_PROPERTIES (starfish.webworker PROPERTIES OUTPUT_NAME ${STARFISH_WEBWORKER_OUTPUT_NAME})
