CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# CONFIG
#######################################################

# DEFINITION Description
# STARFISH_ENABLE_CAST_SERVICE : enable app cast service
# STARFISH_SERVICE_WORKER_HOST: code blocks in this scope are only for sw host.

SET (STARFISH_SERVICEWORKER_DEFINITIONS
    ${LWE_DEFINES_DEFAULT}
    ${LWE_DEFINES_ICU}
    ${LWE_DEFINES_HOST}
    ${LWE_DEFINES_CUSTOM}
    ${LWE_DEFINES_MODE}
    ${SERVICE_WORKER_CXXFLAGS}
    -DPORT_EVENTLOOP_BACKEND_LIBUV
    -DSTARFISH_WEBWORKER_HOST
    -DSTARFISH_SERVICE_WORKER_HOST
    -DSERVICE_WORKER_USE_SEPARATE_PROCESS
    -DPORT_NEEDS_THREADED_PUBLIC_API
)

SET (STARFISH_SERVICEWORKER_LIBRARIES_DEFAULT pthread curl ssl crypto)
SET (STARFISH_SERVICEWORKER_DEPENDENCIES escargot tuv nanomsg generate_binding)
SET (STARFISH_SERVICEWORKER_LIBRARIES_THIRD_PARTY escargot ${GC_TARGET} ${TUV_TARGET} ${NANOMSG_TARGET})
SET (STARFISH_SERVICEWORKER_INCLUDE_ADDITIONAL_DIRS
    ${GCUTIL_ROOT}
    ${GCUTIL_ROOT}/bdwgc/include
    ${ESCARGOT_ROOT}/src/api
    ${ESCARGOT_ROOT}/third_party/rapidjson/include
    ${ESCARGOT_ROOT}/third_party/runtime_icu_binder
    ${ESCARGOT_ROOT}/third_party/rapidjson/include
    ${THIRD_PARTY_ROOT}/robin_map/include
    ${THIRD_PARTY_ROOT}/libtuv/include
    ${THIRD_PARTY_ROOT}/libtuv/src
    ${THIRD_PARTY_ROOT}/nanomsgcpp
    ${THIRD_PARTY_ROOT}/httplib)
SET (STARFISH_SERVICEWORKER_ENTRY ${STARFISH_ROOT}/src/launcher/ServiceWorkerEntry.cpp)

IF (NOT (${BACKEND} STREQUAL "efl_skia_gl" OR ${BACKEND} STREQUAL "efl_skia_gb"))
SET (STARFISH_SERVICEWORKER_DEPENDENCIES  ${STARFISH_SERVICEWORKER_DEPENDENCIES} skia_matrix)
SET (STARFISH_SERVICEWORKER_LIBRARIES_THIRD_PARTY  ${STARFISH_SERVICEWORKER_LIBRARIES_THIRD_PARTY} skia_matrix)
SET (STARFISH_SERVICEWORKER_INCLUDE_ADDITIONAL_DIRS
    ${STARFISH_SERVICEWORKER_INCLUDE_ADDITIONAL_DIRS}
    ${THIRD_PARTY_ROOT}/skia_matrix
    ${THIRD_PARTY_ROOT}/skia_matrix/include/core)
ENDIF()

#######################################################
# SOURCE FILES
#######################################################

# Extract the path of the interface exposed to the Worker
FILE (GLOB_RECURSE STARFISH_IDL ${STARFISH_ROOT}/src/*.idl)
SET (STARFISH_SERVICEWORKER_EXPOSED_INTERFACE_SRC)
# TODO: include this interface or completely exclude in Worker.
SET (EXCLUDE_INTERFACE_NAME
    "Navigator" "EventSource" "FormData"  "CSS")
FOREACH (IDL_FILE ${STARFISH_IDL})
    FILE (READ ${IDL_FILE} IDL_STRING)
    STRING (REGEX MATCH "[[].*Exposed=(.*Worker|.*,.*Worker)" MATCHED_IDL_FILE ${IDL_STRING})
    IF (MATCHED_IDL_FILE)
        STRING (REGEX MATCH "[a-zA-Z0-9]+[.]idl" MATCHED_INTERFACE_NAME ${IDL_FILE})
        IF (MATCHED_INTERFACE_NAME)
            STRING (REPLACE ".idl" "" MATCHED_INTERFACE_NAME ${MATCHED_INTERFACE_NAME})
            LIST (FIND EXCLUDE_INTERFACE_NAME ${MATCHED_INTERFACE_NAME} MATCH_IDX)
            IF (${MATCH_IDX} LESS 0)
                # Add binding source file
                STRING (REPLACE ".idl" ".cpp" SOURCE_FILE ${IDL_FILE})
                IF (EXISTS ${SOURCE_FILE})
                    LIST (APPEND STARFISH_SERVICEWORKER_EXPOSED_INTERFACE_SRC ${SOURCE_FILE})
                ENDIF()
                #Add source file
                SET (INTERFACE_BINDING_SRC ${STARFISH_BINDING_GENERATED_DIR}/${MATCHED_INTERFACE_NAME}Binding.cpp)
                IF (EXISTS ${INTERFACE_BINDING_SRC})
                    LIST (APPEND STARFISH_SERVICEWORKER_EXPOSED_INTERFACE_SRC ${INTERFACE_BINDING_SRC})
                ENDIF()
            ENDIF()
        ENDIF()
    ENDIF()
ENDFOREACH()

FILE (GLOB STARFISH_SERVICEWORKER_DEFAULT_SRC
    ${STARFISH_ROOT}/src/public/LWE.cpp
    ${STARFISH_ROOT}/src/public/LWEServiceWorker.cpp
    ${STARFISH_ROOT}/src/StaticStrings.cpp
    ${STARFISH_ROOT}/src/Starfish.cpp
    ${STARFISH_ROOT}/src/platform/loader/ResourceURL.cpp
    ${STARFISH_ROOT}/src/platform/message_loop/*.cpp
    ${STARFISH_ROOT}/src/platform/network/curl/*.cpp
    ${STARFISH_ROOT}/src/platform/network/http/*.cpp
    ${STARFISH_ROOT}/src/platform/file/*.cpp
    ${STARFISH_ROOT}/src/platform/process/base/*.cpp
    ${STARFISH_ROOT}/src/platform/public/*.cpp
)

FILE (GLOB STARFISH_SERVICEWORKER_CORE_SRC
    ${STARFISH_ROOT}/src/core/util/*.cpp
    ${STARFISH_ROOT}/src/core/fileapi/*.cpp
    ${STARFISH_ROOT}/src/core/extra/Console.cpp
    ${STARFISH_ROOT}/src/core/extra/MimeType.cpp
    ${STARFISH_ROOT}/src/core/page/WebBase.cpp
    ${STARFISH_ROOT}/src/core/page/NavigatorMixin.cpp
    ${STARFISH_ROOT}/src/core/page/Serializer.cpp
    ${STARFISH_ROOT}/src/core/modules/message_loop/*.cpp
    ${STARFISH_ROOT}/src/core/modules/threading/*.cpp
    ${STARFISH_ROOT}/src/core/modules/resource_request/*.cpp
    ${STARFISH_ROOT}/src/core/modules/networking/*.cpp
    ${STARFISH_ROOT}/src/core/modules/worker/util/*.cpp
    ${STARFISH_ROOT}/src/core/modules/worker/host/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/cache/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/host/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/push/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/notification/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/util/*.cpp
    ${STARFISH_ROOT}/src/core/modules/profiling/Profiling.cpp
    ${STARFISH_ROOT}/src/core/modules/cast/*.cpp
    ${STARFISH_ROOT}/src/core/dom/ExecutionContext.cpp
    ${STARFISH_ROOT}/src/core/dom/WebOrigin.cpp
    ${STARFISH_ROOT}/src/core/dom/CloseEvent.cpp
    ${STARFISH_ROOT}/src/core/dom/Event.cpp
    ${STARFISH_ROOT}/src/core/dom/EventTarget.cpp
    ${STARFISH_ROOT}/src/core/dom/DOMException.cpp
    ${STARFISH_ROOT}/src/core/csp/*.cpp
    ${STARFISH_ROOT}/src/core/fetch/*.cpp
    ${STARFISH_ROOT}/src/core/fetch/stream/*.cpp
    ${STARFISH_ROOT}/src/core/storage/StorageInternal*.cpp
    ${STARFISH_ROOT}/src/core/storage/StorageNamespace*.cpp
    ${STARFISH_ROOT}/src/core/storage/StoragePersistent*.cpp
    ${STARFISH_ROOT}/src/core/storage/WebStorage*.cpp
)

FILE (GLOB STARFISH_SERVICEWORKER_BINDING_SRC
    ${STARFISH_ROOT}/src/binding/ScriptWrappable.cpp
    ${STARFISH_ROOT}/src/binding/ScriptEngineInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingWorkerInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingServiceWorkerInstance.cpp
    ${STARFISH_ROOT}/src/binding/BlobCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/EventTargetCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/WorkerGlobalScopeCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/URLSearchParamsCustomBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/RequestInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/BlobOrBufferSourceOrUSVStringOrReadableStreamBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/ArrayBufferViewOrArrayBufferBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/SecurityPolicyViolationEventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/ResponseInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/ErrorEventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/DOMPointInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/CustomEventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/EventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/RequestOrUSVStringBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/RegistrationOptionsBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/WindowOrServiceWorkerBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/MessageEventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/DOMStringOrSequenceBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/PushSubscriptionOptionsInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/BufferSourceOrDOMStringBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/NotificationOptionsBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/DOMMatrix2DInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/TextDecoderOptionsBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/ProgressEventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/DOMStringOrArrayBufferBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/URLSearchParamsBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/CloseEventInitBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/TextDecodeOptionsBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/CustomStorageBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/InternalBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/XMLHttpRequestEventTargetBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/XMLHttpRequestUploadBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/BufferSourceOrBlobOrDOMStringBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/FilePropertyBagBinding.cpp
)

FILE (GLOB STARFISH_SERVICEWORKER_PUBLIC_SRC
    ${STARFISH_ROOT}/src/public/LWEServiceWorker.cpp
)

SET (STARFISH_SERVICEWORKER_SRC_LIST
    ${STARFISH_SERVICEWORKER_DEFAULT_SRC}
    ${STARFISH_SERVICEWORKER_CORE_SRC}
    ${STARFISH_SERVICEWORKER_BINDING_SRC}
    ${STARFISH_SERVICEWORKER_EXPOSED_INTERFACE_SRC}
    ${STARFISH_SERVICEWORKER_PUBLIC_SRC}
)

#######################################################
# INCLUDE DIRS
#######################################################
SET (STARFISH_SERVICEWORKER_INCLUDE_DIRS
    ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS}
    ${STARFISH_ROOT}/src
    ${STARFISH_ROOT}/inc
    ${STARFISH_BINDING_INCLUDE_DIR}
    ${STARFISH_SERVICEWORKER_INCLUDE_ADDITIONAL_DIRS}
)

#######################################################
# LINK LIBRARIES
#######################################################
SET (STARFISH_SERVICEWORKER_LINK_LIBRARIES
    ${STARFISH_THIRD_PARTY_LIBS_LIBRARIES}
    ${STARFISH_SERVICEWORKER_LIBRARIES_THIRD_PARTY}
    ${STARFISH_SERVICEWORKER_LIBRARIES_DEFAULT}
    ${STARFISH_LIBRARIES_COMPILER}
)

#######################################################
# CUSTOM TARGET JS2C
#######################################################
MACRO (add_js2c_target name output source license fname)
    ADD_CUSTOM_COMMAND (OUTPUT ${output}
                       COMMENT "Js2c (${name})"
                       COMMAND ${CMAKE_SOURCE_DIR}/tool/js2c.py -s${source} -l${license} -o${fname}
                       DEPENDS ${source}
    )
    ADD_CUSTOM_TARGET (${name} DEPENDS ${output})
    SET (JS2C_DEPENDENCIES ${JS2C_DEPENDENCIES} ${name})
ENDMACRO()

add_js2c_target(CacheStorage
    "${CMAKE_SOURCE_DIR}/src/binding/generated/Js2c_CacheStorage.h"
    "${CMAKE_SOURCE_DIR}/src/core/modules/serviceworker/cache/deps/cache-storage/dist/cache.min.js"
    "${CMAKE_SOURCE_DIR}/src/core/modules/serviceworker/cache/deps/cache-storage/LICENSE"
    "CacheStorage.h"
)

#######################################################
# BUILD TARGET
#######################################################
SET (STARFISH_SERVICEWORKER_OBJECT_LIBRARY starfish_serviceworker_object_library)
SET (STARFISH_SERVICEWORKER_OUTPUT_NAME ${TARGETNAME}-serviceworker)

ADD_LIBRARY (${STARFISH_SERVICEWORKER_OBJECT_LIBRARY} OBJECT ${STARFISH_SERVICEWORKER_SRC_LIST})
ADD_DEPENDENCIES (${STARFISH_SERVICEWORKER_OBJECT_LIBRARY}
    ${STARFISH_SERVICEWORKER_DEPENDENCIES}
    ${JS2C_DEPENDENCIES})

ADD_EXECUTABLE (starfish.serviceworker.executable
                $<TARGET_OBJECTS:${STARFISH_SERVICEWORKER_OBJECT_LIBRARY}>
                ${STARFISH_SERVICEWORKER_ENTRY})

ADD_CUSTOM_COMMAND (TARGET starfish.serviceworker.executable POST_BUILD
    COMMAND ln -fs ${OUTPUT_DIRECTORY}/bin/${STARFISH_SERVICEWORKER_OUTPUT_NAME} ${STARFISH_ROOT}/Starfish-serviceworker
)

ADD_LIBRARY (starfish.serviceworker.shared_library SHARED $<TARGET_OBJECTS:${STARFISH_SERVICEWORKER_OBJECT_LIBRARY}>)
ADD_LIBRARY (starfish.serviceworker.static_library STATIC $<TARGET_OBJECTS:${STARFISH_SERVICEWORKER_OBJECT_LIBRARY}>)

SET (SERVICEWORKER_CXXFLAGS ${LWE_CXXFLAGS})
SET (SERVICEWORKER_LDFLAGS ${LWE_LDFLAGS})

MESSAGE (STATUS "WebWorker")
MESSAGE (STATUS "FLAGS: " ${SERVICEWORKER_CXXFLAGS})
MESSAGE (STATUS "LIBRARIES: " ${STARFISH_SERVICEWORKER_LINK_LIBRARIES})
MESSAGE (STATUS "DEFINITIONS: " ${STARFISH_SERVICEWORKER_DEFINITIONS})
MESSAGE (STATUS "LDFLAGS: " ${SERVICEWORKER_LDFLAGS})
MESSAGE (STATUS "INCLUDE_DIRS: " ${STARFISH_SERVICEWORKER_INCLUDE_DIRS})

TARGET_INCLUDE_DIRECTORIES (${STARFISH_SERVICEWORKER_OBJECT_LIBRARY} PUBLIC ${STARFISH_SERVICEWORKER_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS (${STARFISH_SERVICEWORKER_OBJECT_LIBRARY} PUBLIC ${STARFISH_SERVICEWORKER_DEFINITIONS})
TARGET_COMPILE_OPTIONS (${STARFISH_SERVICEWORKER_OBJECT_LIBRARY} PUBLIC ${SERVICEWORKER_CXXFLAGS})

TARGET_INCLUDE_DIRECTORIES (starfish.serviceworker.executable PUBLIC ${STARFISH_SERVICEWORKER_INCLUDE_DIRS})
TARGET_COMPILE_DEFINITIONS (starfish.serviceworker.executable PUBLIC ${STARFISH_SERVICEWORKER_DEFINITIONS})
TARGET_COMPILE_OPTIONS (starfish.serviceworker.executable PUBLIC ${SERVICEWORKER_CXXFLAGS})

TARGET_LINK_LIBRARIES (starfish.serviceworker.executable PRIVATE ${STARFISH_SERVICEWORKER_LINK_LIBRARIES} ${SERVICEWORKER_LDFLAGS})
TARGET_LINK_LIBRARIES (starfish.serviceworker.shared_library PRIVATE ${STARFISH_SERVICEWORKER_LINK_LIBRARIES} ${SERVICEWORKER_LDFLAGS})
TARGET_LINK_LIBRARIES (starfish.serviceworker.static_library PRIVATE ${STARFISH_SERVICEWORKER_LINK_LIBRARIES} ${SERVICEWORKER_LDFLAGS})

SET_TARGET_PROPERTIES (starfish.serviceworker.executable PROPERTIES OUTPUT_NAME ${STARFISH_SERVICEWORKER_OUTPUT_NAME})
SET_TARGET_PROPERTIES (starfish.serviceworker.shared_library PROPERTIES OUTPUT_NAME ${STARFISH_SERVICEWORKER_OUTPUT_NAME})
SET_TARGET_PROPERTIES (starfish.serviceworker.shared_library PROPERTIES OUTPUT_NAME ${STARFISH_SERVICEWORKER_OUTPUT_NAME})
