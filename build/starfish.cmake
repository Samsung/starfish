CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# SOURCE FILES
#######################################################
FILE (GLOB_RECURSE STARFISH_SRC ${STARFISH_ROOT}/src/*.cpp)

IF (${ARCH} STREQUAL "tizen")
    FILE (GLOB STARFISH_SRC_EXTRA ${THIRD_PARTY_ROOT}/deviceapi/src/*.cpp)
ENDIF()


IF (${CUSTOM} STREQUAL "prod_tv")
    SET (STARFISH_SRC_CUSTOM
        ${STARFISH_ROOT}/src/platform/tts/TTSBase.cpp
        ${STARFISH_ROOT}/src/platform/multimedia/MediaPlayerTizenBase.cpp)
ENDIF()

SET (STARFISH_SRC_LIST
    ${STARFISH_SRC}
    ${STARFISH_SRC_CUSTOM}
    ${STARFISH_SRC_EXTRA}
)

#######################################################
# INCLUDE DIRS
#######################################################

SET (STARFISH_INCLUDE_DIRS
    ${STARFISH_INCLUDE_DIRS_DEFAULT}
    ${STARFISH_BACKEND_INCLUDE_DIRS}
    ${STARFISH_INCLUDE_DIRS_CUSTOM}
    ${STARFISH_INCLUDE_ADDITIONAL_DIRS}
    ${GCUTIL_ROOT}
    ${GCUTIL_ROOT}/bdwgc/include
    ${ESCARGOT_ROOT}/include
    ${ESCARGOT_ROOT}/src/api
    ${THIRD_PARTY_ROOT}/skia_matrix
    ${THIRD_PARTY_ROOT}/MP4Parse/source/include
    ${THIRD_PARTY_ROOT}/webm
    ${THIRD_PARTY_ROOT}/zeromq/include
    ${THIRD_PARTY_ROOT}/cppzmq
    ${THIRD_PARTY_ROOT}/clipper/cpp
    ${THIRD_PARTY_ROOT}/earcut.hpp/include/mapbox
    ${STARFISH_TIZEN_CUSTOM_INCLUDE_DIRS}
    ${STARFISH_TIZEN_INCLUDE_DIRS}
    ${STARFISH_ADDTIONAL_INCLUDE_DIRS}
)

#######################################################
# LINK LIBRARIES
#######################################################
SET (STARFISH_LINK_LIBRARIES
    ${STARFISH_LIBRARIES_DEFAULT}
    ${STARFISH_LIBRARIES_COMPILER}
    ${STARFISH_BACKEND_LIBRARIES}
    ${STARFISH_LIBRARIES_BACKEND}
    ${STARFISH_LIBRARIES_ARCH}
    ${STARFISH_LIBRARIES_CUSTOM}
    ${STARFISH_LIBRARIES_TOUCH_UI}
    skia_matrix
    clipper
    mp4parse
    webm
    tuv
    gc
    escargot
    ${STARFISH_TIZEN_CUSTOM_LIBRARIES}
    ${STARFISH_DALI_ADDTIONAL_LIBRARIES}
)

#######################################################
# BUILD TARGET
#######################################################
IF (${ARCH} STREQUAL "tizen")
    SET (STARFISH_SRC_LIST ${STARFISH_SRC_LIST} ${TUV_LIB})
    SET (STARFISH_LINK_LIBRARIES ${STARFISH_LINK_LIBRARIES} ${TUV_LIB})
ELSE()
    SET (STARFISH_SRC_LIST ${STARFISH_SRC_LIST})
    SET (STARFISH_LINK_LIBRARIES ${STARFISH_LINK_LIBRARIES})
ENDIF()

IF (${COMPONENT} STREQUAL "executable")
    ADD_EXECUTABLE (${TARGETNAME} ${STARFISH_SRC_LIST})
    ADD_CUSTOM_COMMAND (TARGET ${TARGETNAME} POST_BUILD
        COMMAND ln -fs ${OUTPUT_DIRECTORY}/bin/${TARGETNAME} ${STARFISH_ROOT}/${TARGETNAME}
    )
ELSEIF (${COMPONENT} STREQUAL "shared_library")
    ADD_LIBRARY (${TARGETNAME} SHARED ${STARFISH_SRC_LIST})
ELSEIF (${COMPONENT} STREQUAL "static_library")
    ADD_LIBRARY (${TARGETNAME} STATIC ${STARFISH_SRC_LIST})
ENDIF()

ADD_DEPENDENCIES (${TARGETNAME}
    skia_matrix
    clipper
    mp4parse
    webm
    tuv
    gc
    escargot
)

#######################################################
# STARFISH TARGET
#######################################################

TARGET_INCLUDE_DIRECTORIES (${TARGETNAME} PUBLIC ${STARFISH_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES (${TARGETNAME} ${STARFISH_LINK_LIBRARIES})
TARGET_COMPILE_DEFINITIONS (${TARGETNAME} PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (${TARGETNAME} PUBLIC ${LWE_CXXFLAGS})
IF (${HOST} STREQUAL "linux")
    SET_TARGET_PROPERTIES (${TARGETNAME} PROPERTIES LINK_FLAGS ${LWE_LDFLAGS})
ENDIF()
