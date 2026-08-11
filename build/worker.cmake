#######################################################
# Workers have the following structure:
#
# Dedicated Worker: thread model
# Shared Worker: process model
# Service Worker: process model
#
# The process model has two processes: Starfish and Host(ServiceWorkerHost or SharedWorkerHost).
# This makes the host target for service and shared worker.
#
# Each process has definitions as follows. Below XXX is SHARED or SERVICE.
# Starfish(client): STARFISH_ENABLE_WORKER, STARFISH_ENABLE_XXX_WORKER, STARFISH_USE_WORKER_PROCESS
# Host(server): STARFISH_ENABLE_WORKER, STARFISH_ENABLE_XXX_WORKER, STARFISH_USE_WORKER_PROCESS, STARFISH_WEBWORKER_HOST
#
# DEFINITION Description
# STARFISH_ENABLE_WORKER: code blocks in this scope are only for dedicated, shared and service worker.
# STARFISH_ENABLE_XXX_WORKER: code blocks in this scope are only for xxx worker.
# STARFISH_WEBWORKER_HOST: code blocks in this scope are only for host(shared, service).
# STARFISH_USE_WORKER_PROCESS: code blocks in this scope are only for process model(shared, service).
#
#######################################################

CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# CONFIG
#######################################################
SET (STARFISH_WORKER_DEFINITIONS
    ${LWE_DEFINES_DEFAULT}
    ${LWE_DEFINES_ICU}
    ${LWE_DEFINES_HOST}
    ${LWE_DEFINES_MODE}
    -DPORT_EVENTLOOP_BACKEND_LIBUV
    -DPORT_NEEDS_THREADED_PUBLIC_API
    -DSTARFISH_ENABLE_WORKER
    -DSTARFISH_WEBWORKER_HOST
    -DSTARFISH_USE_WORKER_PROCESS
    -DSTARFISH_BACKEND_STR="uv_worker"
)

LIST (REMOVE_ITEM STARFISH_WORKER_DEFINITIONS
    -DTIZEN_DEVICE_API
)

SET (STARFISH_SHARED_WORKER_DEFINITIONS
    ${STARFISH_WORKER_DEFINITIONS}
    -DSTARFISH_ENABLE_SHARED_WORKER
)

SET (STARFISH_SERVICE_WORKER_DEFINITIONS
    ${STARFISH_WORKER_DEFINITIONS}
    ${SERVICE_WORKER_CXXFLAGS}
    -DSTARFISH_ENABLE_SERVICE_WORKER
)

SET (STARFISH_WORKER_LIBRARIES_DEFAULT pthread curl ssl crypto)
SET (STARFISH_WORKER_DEPENDENCIES ${STARFISH_DEPENDENCIES})
SET (STARFISH_WORKER_LIBRARIES_THIRD_PARTY escargot ${GC_TARGET} ${TUV_TARGET} ${NANOMSG_TARGET})

SET (STARFISH_WORKER_INCLUDE_ADDITIONAL_DIRS
    ${GCUTIL_ROOT}
    ${GCUTIL_ROOT}/include
    ${GCUTIL_ROOT}/include/gc

    ${ESCARGOT_ROOT}/src/api
    ${ESCARGOT_ROOT}/third_party/runtime_icu_binder
    ${THIRD_PARTY_ROOT}/robin_map/include
    ${THIRD_PARTY_ROOT}/libtuv/include
    ${THIRD_PARTY_ROOT}/libtuv/src
    ${THIRD_PARTY_ROOT}/nanomsgcpp
    ${THIRD_PARTY_ROOT}/httplib
    ${THIRD_PARTY_ROOT}/skia_matrix
    ${THIRD_PARTY_ROOT}/skia_matrix/include/core
    ${STARFISH_LIBWEBSOCKETS_ADDITIONAL_INCLUDE_DIRS}
)

SET (STARFISH_WORKER_DEPENDENCIES ${STARFISH_WORKER_DEPENDENCIES} skia_matrix)
SET (STARFISH_WORKER_LIBRARIES_THIRD_PARTY ${STARFISH_WORKER_LIBRARIES_THIRD_PARTY} skia_matrix)

SET (STARFISH_SHARED_WORKER_DEPENDENCIES ${STARFISH_DEPENDENCIES})
SET (STARFISH_SERVICE_WORKER_DEPENDENCIES ${STARFISH_DEPENDENCIES})

#######################################################
# PACKAGE
#######################################################

IF (${HOST} STREQUAL "tizen")
    pkg_check_modules (STARFISH_WORKER_TIZEN_PACKAGE REQUIRED dlog capi-appfw-app-common)
ENDIF()

#######################################################
# SOURCE FILES
#######################################################

# Extract the path of the interface exposed to the Worker
FILE (GLOB_RECURSE STARFISH_IDL ${STARFISH_ROOT}/src/*.idl)
SET (STARFISH_WORKER_EXPOSED_INTERFACE_SRC)

# TODO: include this interface or completely exclude in Worker.
SET (EXCLUDE_INTERFACE_NAME
    "Navigator" "EventSource" "FormData"  "CSS" "Worker")

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
                    LIST (APPEND STARFISH_WORKER_EXPOSED_INTERFACE_SRC ${SOURCE_FILE})
                ENDIF()
                #Add source file
                SET (INTERFACE_BINDING_SRC ${STARFISH_BINDING_GENERATED_DIR}/${MATCHED_INTERFACE_NAME}Binding.cpp)
                IF (EXISTS ${INTERFACE_BINDING_SRC})
                    LIST (APPEND STARFISH_WORKER_EXPOSED_INTERFACE_SRC ${INTERFACE_BINDING_SRC})
                ENDIF()
            ENDIF()
        ENDIF()
    ENDIF()
ENDFOREACH()

FILE (GLOB STARFISH_WORKER_DEFAULT_SRC
    ${STARFISH_ROOT}/src/public/delegate/LWEDelegate.cpp
    ${STARFISH_ROOT}/src/public/delegate/ThreadedCallHelper.cpp
    ${STARFISH_ROOT}/src/public/delegate/LWEWorkerDelegate.cpp
    ${STARFISH_ROOT}/src/StaticStrings.cpp
    ${STARFISH_ROOT}/src/StoragePathProvider.cpp
    ${STARFISH_ROOT}/src/Starfish.cpp
    ${STARFISH_ROOT}/src/platform/loader/ResourceURL.cpp
    ${STARFISH_ROOT}/src/platform/message_loop/*.cpp
    ${STARFISH_ROOT}/src/platform/network/curl/*.cpp
    ${STARFISH_ROOT}/src/platform/network/http/*.cpp
    ${STARFISH_ROOT}/src/platform/file/*.cpp
    ${STARFISH_ROOT}/src/platform/process/base/*.cpp
)

FILE (GLOB STARFISH_WORKER_CORE_SRC
    ${STARFISH_ROOT}/src/core/util/*.cpp
    ${STARFISH_ROOT}/src/core/fileapi/*.cpp
    ${STARFISH_ROOT}/src/core/util/debug/*.cpp
    ${STARFISH_ROOT}/src/core/extra/Console.cpp
    ${STARFISH_ROOT}/src/core/extra/MimeType.cpp
    ${STARFISH_ROOT}/src/core/page/WebBase.cpp
    ${STARFISH_ROOT}/src/core/page/NavigatorMixin.cpp
    ${STARFISH_ROOT}/src/core/serialize/*.cpp
    ${STARFISH_ROOT}/src/core/modules/message_loop/*.cpp
    ${STARFISH_ROOT}/src/core/modules/threading/*.cpp
    ${STARFISH_ROOT}/src/core/modules/resource_request/*.cpp
    ${STARFISH_ROOT}/src/core/modules/networking/*.cpp
    ${STARFISH_ROOT}/src/core/modules/worker/*.cpp
    ${STARFISH_ROOT}/src/core/modules/worker/util/*.cpp
    ${STARFISH_ROOT}/src/core/modules/worker/util/network/*.cpp
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

FILE (GLOB STARFISH_SHARED_WORKER_CORE_SRC
    ${STARFISH_ROOT}/src/core/modules/sharedworker/*.cpp
    ${STARFISH_ROOT}/src/core/modules/sharedworker/host/*.cpp
)

FILE (GLOB STARFISH_SERVICE_WORKER_CORE_SRC
    ${STARFISH_ROOT}/src/core/modules/serviceworker/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/cache/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/host/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/push/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/notification/*.cpp
    ${STARFISH_ROOT}/src/core/modules/serviceworker/util/*.cpp
)

FILE (GLOB STARFISH_WORKER_BINDING_SRC
    ${STARFISH_ROOT}/src/binding/ScriptWrappable.cpp
    ${STARFISH_ROOT}/src/binding/ScriptEngineInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingInstance.cpp
    ${STARFISH_ROOT}/src/binding/ScriptBindingWorkerInstance.cpp
    ${STARFISH_ROOT}/src/binding/EventTargetCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/WorkerGlobalScopeCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/URLSearchParamsCustomBinding.cpp
    ${STARFISH_ROOT}/src/binding/XMLHttpRequestCustomBinding.cpp
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
    ${STARFISH_BINDING_GENERATED_DIR}/WindowOrMessagePortOrServiceWorkerBinding.cpp
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
    ${STARFISH_BINDING_GENERATED_DIR}/BlobPropertyBagBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/FilePropertyBagBinding.cpp
    ${STARFISH_BINDING_GENERATED_DIR}/StructuredSerializeOptionsBinding.cpp
)

FILE (GLOB STARFISH_WORKER_PUBLIC_SRC
    ${STARFISH_ROOT}/src/public/LWEWorker.cpp
)

SET (STARFISH_WORKER_SRC_LIST
    ${STARFISH_WORKER_DEFAULT_SRC}
    ${STARFISH_WORKER_CORE_SRC}
    ${STARFISH_WORKER_BINDING_SRC}
    ${STARFISH_WORKER_EXPOSED_INTERFACE_SRC}
)

SET (STARFISH_SHARED_WORKER_SRC_LIST
    ${STARFISH_WORKER_SRC_LIST}
    ${STARFISH_SHARED_WORKER_CORE_SRC}
    ${STARFISH_WORKER_PUBLIC_SRC}
)

SET (STARFISH_SERVICE_WORKER_SRC_LIST
    ${STARFISH_WORKER_SRC_LIST}
    ${STARFISH_SERVICE_WORKER_CORE_SRC}
    ${STARFISH_WORKER_PUBLIC_SRC}
)

#######################################################
# INCLUDE DIRS
#######################################################
SET (STARFISH_WORKER_INCLUDE_DIRS
    ${STARFISH_INCLUDE_DIRS_DEFAULT}
    ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS}
    ${STARFISH_BINDING_INCLUDE_DIR}
    ${STARFISH_WORKER_INCLUDE_ADDITIONAL_DIRS}
    ${STARFISH_WORKER_TIZEN_PACKAGE_INCLUDE_DIRS}
)

SET (STARFISH_SHARED_WORKER_INCLUDE_DIRS
    ${STARFISH_WORKER_INCLUDE_DIRS}
)

SET(STARFISH_SERVICE_WORKER_INCLUDE_DIRS
    ${STARFISH_WORKER_INCLUDE_DIRS}
)

#######################################################
# LINK LIBRARIES
#######################################################
SET (STARFISH_WORKER_LINK_LIBRARIES
    ${STARFISH_LIBRARIES_HOST}
    ${STARFISH_THIRD_PARTY_LIBS_LIBRARIES}
    ${STARFISH_WORKER_LIBRARIES_THIRD_PARTY}
    ${STARFISH_WORKER_LIBRARIES_DEFAULT}
    ${STARFISH_LIBRARIES_COMPILER}
    ${STARFISH_WORKER_TIZEN_PACKAGE_LIBRARIES}
)

SET (STARFISH_SHARED_WORKER_LINK_LIBRARIES
    ${STARFISH_WORKER_LINK_LIBRARIES}
)

SET (STARFISH_SERVICE_WORKER_LINK_LIBRARIES
    ${STARFISH_WORKER_LINK_LIBRARIES}
)

#######################################################
# CUSTOM TARGET JS2C
#######################################################
MACRO (add_js2c_target name output source license)
    ADD_CUSTOM_COMMAND (OUTPUT ${output}
                       COMMENT "Js2c (${name})"
                       COMMAND ${CMAKE_SOURCE_DIR}/tool/build/js2c.py -s${source} -l${license} -o${output}
                       DEPENDS ${source}
    )
    ADD_CUSTOM_TARGET (${name} DEPENDS ${output})
    SET (JS2C_DEPENDENCIES ${JS2C_DEPENDENCIES} ${name})
ENDMACRO()

add_js2c_target (CacheStorage
    "${STARFISH_BINDING_GENERATED_DIR}/Js2c_CacheStorage.h"
    "${CMAKE_SOURCE_DIR}/src/core/modules/serviceworker/cache/deps/cache-storage/dist/cache.min.js"
    "${CMAKE_SOURCE_DIR}/src/core/modules/serviceworker/cache/deps/cache-storage/LICENSE"
)

SET (STARFISH_SERVICE_WORKER_DEPENDENCIES ${STARFISH_SERVICE_WORKER_DEPENDENCIES} ${JS2C_DEPENDENCIES})

#######################################################
# BUILD TARGET
#######################################################
SET (STARFISH_WORKER_CXXFLAGS ${LWE_CXXFLAGS})
SET (STARFISH_WORKER_LDFLAGS ${LWE_LDFLAGS})

MACRO (add_worker_target file_name variable_name)
    SET (STARFISH_${variable_name}_OBJECT_LIBRARY starfish_${file_name}_object_library)

    ADD_LIBRARY (${STARFISH_${variable_name}_OBJECT_LIBRARY} OBJECT ${STARFISH_${variable_name}_SRC_LIST})
    ADD_DEPENDENCIES (${STARFISH_${variable_name}_OBJECT_LIBRARY} ${STARFISH_${variable_name}_DEPENDENCIES})


    ADD_LIBRARY (starfish.${file_name}.shared_library SHARED $<TARGET_OBJECTS:${STARFISH_${variable_name}_OBJECT_LIBRARY}>)
    ADD_LIBRARY (starfish.${file_name}.static_library STATIC $<TARGET_OBJECTS:${STARFISH_${variable_name}_OBJECT_LIBRARY}>)

    MESSAGE (STATUS ${variable_name})
    MESSAGE (STATUS "FLAGS: " "${STARFISH_WORKER_CXXFLAGS}")
    MESSAGE (STATUS "LIBRARIES: " "${STARFISH_${variable_name}_LINK_LIBRARIES}")
    MESSAGE (STATUS "DEFINITIONS: " "${STARFISH_${variable_name}_DEFINITIONS}")
    MESSAGE (STATUS "LDFLAGS: " "${STARFISH_WORKER_LDFLAGS}")
    MESSAGE ("")

    TARGET_INCLUDE_DIRECTORIES (${STARFISH_${variable_name}_OBJECT_LIBRARY}
        PUBLIC ${STARFISH_${variable_name}_INCLUDE_DIRS})
    TARGET_COMPILE_DEFINITIONS (${STARFISH_${variable_name}_OBJECT_LIBRARY}
        PUBLIC ${STARFISH_${variable_name}_DEFINITIONS})
    TARGET_COMPILE_OPTIONS (${STARFISH_${variable_name}_OBJECT_LIBRARY}
        PUBLIC ${STARFISH_WORKER_CXXFLAGS})

    TARGET_LINK_LIBRARIES (starfish.${file_name}.shared_library
        PUBLIC ${STARFISH_${variable_name}_LINK_LIBRARIES} ${STARFISH_WORKER_LDFLAGS})
    TARGET_LINK_LIBRARIES (starfish.${file_name}.static_library
        PRIVATE ${STARFISH_${variable_name}_LINK_LIBRARIES} ${STARFISH_WORKER_LDFLAGS})

    SET_TARGET_PROPERTIES (starfish.${file_name}.shared_library PROPERTIES
        OUTPUT_NAME ${TARGETNAME}-${file_name}-impl)
    SET_TARGET_PROPERTIES (starfish.${file_name}.static_library PROPERTIES
        OUTPUT_NAME ${TARGETNAME}-${file_name}-impl)

ENDMACRO()

IF (${SHARED_WORKER} STREQUAL "1")
    CONFIGURE_FILE(${STARFISH_ROOT}/lightweight-web-engine-sharedworker.pc.in
        lightweight-web-engine-sharedworker.pc @ONLY)

    add_worker_target (sharedworker SHARED_WORKER)
ENDIF()

IF (${SERVICE_WORKER} STREQUAL "1")
    CONFIGURE_FILE(${STARFISH_ROOT}/lightweight-web-engine-serviceworker.pc.in
        lightweight-web-engine-serviceworker.pc @ONLY)

    add_worker_target (serviceworker SERVICE_WORKER)
ENDIF()
