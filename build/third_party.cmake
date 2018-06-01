CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# THIRD PARTY
#######################################################

# JS BINDING
EXECUTE_PROCESS (
    COMMAND python ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${OUTPUT_DIRECTORY}/tmp
)

FILE (GLOB STARFISH_BINDING_TMP_LIST ${OUTPUT_DIRECTORY}/tmp/*)

FOREACH ( F ${STARFISH_BINDING_TMP_LIST})
    GET_FILENAME_COMPONENT ("${F}" FN NAME)
    CONFIGURE_FILE ("${F}" "${STARFISH_ROOT}/src/binding/${FN}" COPYONLY)
ENDFOREACH()

FILE (GLOB STARFISH_BINDING_OUTPUT_LIST ${STARFISH_ROOT}/src/binding/*.cpp)

# SKIA
FILE (GLOB SKIA_LIST ${THIRD_PARTY_ROOT}/skia_matrix/*.cpp)
ADD_LIBRARY (skia STATIC ${SKIA_LIST})
TARGET_COMPILE_DEFINITIONS (skia PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (skia PUBLIC ${LWE_CXXFLAGS})

# CLIPPER
ADD_LIBRARY (clipper STATIC ${THIRD_PARTY_ROOT}/clipper/cpp/clipper.cpp)
TARGET_COMPILE_DEFINITIONS (clipper PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (clipper PUBLIC ${LWE_CXXFLAGS})

# ZEROMQ, ESCARGOT, GC and LIBTUV
ADD_LIBRARY (escargot STATIC IMPORTED)
ADD_LIBRARY (gc SHARED IMPORTED)
IF (${ARCH} STREQUAL "tizen")
    SET (ESCARGOT_OUTPUT ${ESCARGOT_ROOT}/libescargot.a)
    SET (GC_OUTPUT ${GCUTIL_ROOT}/bdwgc/out/tizen_obs/arm/${MODE}.shared/.libs/libgc.so)
    SET (LIBTUV_OUTPUT ${THIRD_PARTY_ROOT}/libtuv/build/armv7l-linux/debug/lib/libtuv.so)
    ADD_LIBRARY (libtuv STATIC IMPORTED)
    SET_PROPERTY (TARGET libtuv PROPERTY IMPORTED_LOCATION ${LIBTUV_OUTPUT})
    SET (THIRD_PARTY_ARCH "arm")
ELSE()
    SET (ESCARGOT_OUTPUT ${ESCARGOT_ROOT}/out/${HOST}/${ARCH}/interpreter/${MODE}/libescargot.a)
    SET (GC_OUTPUT ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so)
    SET (ZMQ_OUTPUT ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so)
    ADD_LIBRARY (zmq STATIC IMPORTED)
    SET_PROPERTY (TARGET zmq PROPERTY IMPORTED_LOCATION ${ZMQ_OUTPUT})
    SET (THIRD_PARTY_ARCH "x64")
ENDIF()

SET_PROPERTY (TARGET escargot PROPERTY IMPORTED_LOCATION ${ESCARGOT_OUTPUT})
SET_PROPERTY (TARGET gc PROPERTY IMPORTED_LOCATION ${GC_OUTPUT})
MESSAGE (STATUS "HELL: ${ESCARGOT_OUTPUT}")

#ADD_CUSTOM_COMMAND (OUTPUT ${ESCARGOT_OUTPUT} ${GC_OUTPUT} ${ZMQ_OUTPUT} ${LIBTUV_OUTPUT}
#                    COMMAND @./build_third_party.sh ${THIRD_PARTY_ARCH})

# MP4PARSE
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
ADD_LIBRARY (mp4parse STATIC ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PUBLIC ${LWE_CXXFLAGS})

# WEBM
ADD_LIBRARY (webm STATIC 
    ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
    ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
)
TARGET_COMPILE_DEFINITIONS (webm PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (webm PUBLIC ${LWE_CXXFLAGS})
TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)

#######################################################
# COPY THIRD PARTY LIBRARY FILES
#######################################################

IF (${ARCH} STREQUAL "tizen")
    ADD_CUSTOM_TARGET (library_build
        DEPENDS gc escargot libtuv
        COMMAND ./build_third_party.sh arm
    )
    ADD_CUSTOM_TARGET (library_copy
        DEPENDS library_build
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIRECTORY}/lib/tizen/
        #COMMAND ${CMAKE_COMMAND} -E copy ${STARFISH_ROOT}/tizen_dep/arm/libzmq.a ${OUTPUT_DIRECTORY}/lib/tizen
        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/tizen_obs/arm/${MODE}.shared/.libs/libgc.so.1.0.3  ${OUTPUT_DIRECTORY}/lib/tizen/
        COMMAND ${CMAKE_COMMAND} -E copy ${ESCARGOT_ROOT}/libescargot.a ${OUTPUT_DIRECTORY}/lib/tizen/
        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/libtuv/build/noarch-tizen/debug/lib/libtuv.so ${OUTPUT_DIRECTORY}/lib/tizen
    )
ELSE()
    ADD_CUSTOM_TARGET (library_build
        DEPENDS gc escargot zmq
        COMMAND ./build_third_party.sh x86
    )
    ADD_CUSTOM_TARGET (library_copy
        DEPENDS library_build
        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.so.5.0.1 ${OUTPUT_DIRECTORY}/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libzmq.a ${OUTPUT_DIRECTORY}/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared/.libs/libgc.so.1.0.3  ${OUTPUT_DIRECTORY}/lib/
        COMMAND ${CMAKE_COMMAND} -E copy ${ESCARGOT_ROOT}/out/${HOST}/${ARCH}/interpreter/${MODE}/libescargot.a ${OUTPUT_DIRECTORY}/lib/
        # COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/libtuv/build/x86_64-linux/debug/lib/libtuv.so ${OUTPUT_DIRECTORY}/lib/
    )
ENDIF()
