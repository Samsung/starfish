CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# ESCARGOT THIRDPARTY
IF (${HOST} STREQUAL "linux")
# GIT SUBMODULE
    EXECUTE_PROCESS (
        WORKING_DIRECTORY ${STARFISH_ROOT}
        COMMAND git submodule update --init
    )
    EXECUTE_PROCESS (
        WORKING_DIRECTORY ${ESCARGOT_ROOT}
        COMMAND git submodule update --init third_party
    )
# JS BINDING
    EXECUTE_PROCESS (
        COMMAND python ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${STARFISH_ROOT}/src/binding
    )
ENDIF()

#######################################################
# THIRD PARTY
#######################################################
SET (THIRD_PARTY_CXXFLAGS_COMMON -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fno-omit-frame-pointer -fstack-protector -fPIC)

SET (THIRD_PARTY_CXXFLAGS ${THIRD_PARTY_CXXFLAGS_COMMON} ${LWE_CXXFLAGS_COMPILER} ${CXXFLAGS_FROM_ENV} ${LWE_CXXFLAGS_MODE})
SET (THIRD_PARTY_DEFINITIONS ${LWE_DEFINES_MODE})


#######################################################
# SKIA_MATRIX
#######################################################
IF (NOT ${BACKEND} STREQUAL "efl_skia")
    FILE (GLOB_RECURSE SKIA_MATRIX_SRC_CORE ${THIRD_PARTY_ROOT}/skia_matrix/src/core/*.cpp)
    FILE (GLOB_RECURSE SKIA_MATRIX_SRC_PORTS ${THIRD_PARTY_ROOT}/skia_matrix/src/ports/*.cpp)
    ADD_LIBRARY (skia_matrix SHARED ${SKIA_MATRIX_SRC_CORE} ${SKIA_MATRIX_SRC_PORTS})
    TARGET_INCLUDE_DIRECTORIES (skia_matrix PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix ${THIRD_PARTY_ROOT}/skia_matrix/include/core ${THIRD_PARTY_ROOT}/skia_matrix/include/private)
    TARGET_COMPILE_DEFINITIONS (skia_matrix PUBLIC ${THIRD_PARTY_DEFINITIONS})
    TARGET_COMPILE_OPTIONS (skia_matrix PUBLIC ${THIRD_PARTY_CXXFLAGS})
ENDIF()


#######################################################
# CLIPPER
#######################################################
ADD_LIBRARY (clipper SHARED ${THIRD_PARTY_ROOT}/clipper/cpp/clipper.cpp)
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PUBLIC ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (clipper PUBLIC ${THIRD_PARTY_CXXFLAGS})


#######################################################
# MP4PARSE
#######################################################
IF (NOT ${CUSTOM} MATCHES "wearable")
    FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
    ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
    TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
    TARGET_COMPILE_DEFINITIONS (mp4parse PUBLIC ${THIRD_PARTY_DEFINITIONS})
    TARGET_COMPILE_OPTIONS (mp4parse PUBLIC ${THIRD_PARTY_CXXFLAGS})
ENDIF()


#######################################################
# WEBM
#######################################################
IF (NOT ${CUSTOM} MATCHES "wearable")
    ADD_LIBRARY (webm SHARED
        ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
        ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
    )
    TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)
    TARGET_COMPILE_DEFINITIONS (webm PUBLIC ${THIRD_PARTY_DEFINITIONS})
    TARGET_COMPILE_OPTIONS (webm PUBLIC ${THIRD_PARTY_CXXFLAGS})
ENDIF()


#######################################################
# ZERO MQ
#######################################################
# zmq is not used for tizen
IF (NOT ${HOST} STREQUAL "tizen")
    SET (ZMQ_CFLAGS_COMMON "-g3 -fPIC -I${THIRD_PARTY_ROOT}/zeromq/tweetnacl/contrib/randombytes -I${THIRD_PARTY_ROOT}/zeromq/tweetnacl/src")
    IF (${CUSTOM} STREQUAL "unified_wearable")
        SET (ZMQ_CFLAGS_CUSTOM "-Os")
    ENDIF()
    
    IF (${ARCH} STREQUAL "x86")
        SET (ZMQ_CFLAGS_ARCH "-m32")
    ELSEIF (${ARCH} STREQUAL "arm")
        SET (ZMQ_CFLAGS_ARCH "-march=armv7-a -mthumb -finline-limit=64")
    ENDIF()
    
    IF (${MODE} STREQUAL "debug")
        SET (ZMQ_CFLAGS_MODE "-O0")
    ELSE()
        SET (ZMQ_CFLAGS_MODE "-O2")
    ENDIF()
    
    SET (ZMQ_CFLAGS "${ZMQ_CFLAGS_COMMON} ${ZMQ_CFLAGS_CUSTOM} ${ZMQ_CFLAGS_ARCH} ${ZMQ_CFLAGS_MODE}")
    
    SET (ZMQ_BUILDDIR ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared)
    SET (ZMQ_TARGET ${ZMQ_BUILDDIR}/.libs/libzmq.so)
    
    ADD_CUSTOM_COMMAND (OUTPUT ${ZMQ_TARGET}
                        COMMENT "BUILD ZMQ"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${ZMQ_BUILDDIR}
                        COMMAND cd ${ZMQ_BUILDDIR} && ../../../../configure --enable-static CFLAGS=${ZMQ_CFLAGS} LDFLAGS=${ZMQ_CFLAGS} CXXFLAGS=${ZMQ_CFLAGS}
                        COMMAND cd ${ZMQ_BUILDDIR} && make -j
                        COMMAND ${CMAKE_COMMAND} -E copy ${ZMQ_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
                        
    )

    ADD_CUSTOM_TARGET (zmq
                       DEPENDS ${ZMQ_TARGET}
                       COMMAND echo "ZMQ TARGET"
    )

#    EXECUTE_PROCESS (
#            COMMAND ${CMAKE_COMMAND} -E make_directory ${ZMQ_BUILDDIR}
#    )
#    EXECUTE_PROCESS (
#            WORKING_DIRECTORY ${ZMQ_BUILDDIR}
#            COMMAND ../../../../configure --enable-static CFLAGS=${ZMQ_CFLAGS} LDFLAGS=${ZMQ_CFLAGS} CXXFLAGS=${ZMQ_CFLAGS}
#    )
#    EXECUTE_PROCESS (
#            WORKING_DIRECTORY ${ZMQ_BUILDDIR}
#            COMMAND make -j
#    )
#    
#    ADD_LIBRARY (zmq SHARED IMPORTED)
#    SET_PROPERTY (TARGET zmq PROPERTY
#            IMPORTED_LOCATION ${ZMQ_BUILDDIR}/.libs/libzmq.so
#    )
ENDIF()


#######################################################
# LIBTUV
#######################################################
IF (${ARCH} STREQUAL "x64" AND (${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "glfw_cairo_gl"))
    SET (TUV_DIR ${THIRD_PARTY_ROOT}/libtuv)
    SET (TUV_TARGET ${TUV_DIR}/build/x86_64-linux/${MODE}/lib/libtuv.a)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        COMMAND make clean
                        COMMAND make -j TUV_BUILD_TYPE=${MODE} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=x86_64-linux
    )

    ADD_CUSTOM_TARGET (tuv
                       DEPENDS ${TUV_TARGET}
                       COMMAND echo "TUV TARGET"
    )
ELSEIF (${HOST} STREQUAL "tizen" AND (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali"))
    SET (TUV_DIR ${THIRD_PARTY_ROOT}/libtuv)
    SET (TUV_TARGET ${TUV_DIR}/build/noarch-tizen/${MODE}/lib/libtuv.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        COMMAND make clean
                        COMMAND ${CMAKE_COMMAND} -E copy ${TUV_DIR}/config/tizen/packaging/libtuv.pc.in .
                        COMMAND make -j TUV_BUILD_TYPE=${MODE} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=noarch-tizen
                        COMMAND ${CMAKE_COMMAND} -E copy ${TUV_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (tuv
                       DEPENDS ${TUV_TARGET}
                       COMMAND echo "TUV TARGET"
    )
ENDIF()


#######################################################
# LIBSKIA
#######################################################
IF (${HOST} STREQUAL "linux" AND ${BACKEND} STREQUAL "efl_skia")
    SET (BUILD_TYPE "Release")
    IF (${MODE} STREQUAL "debug")
        SET (BUILD_TYPE "Debug")
    ENDIF()
    ADD_CUSTOM_COMMAND (OUTPUT ${OUTPUT_DIRECTORY}/lib/libskia.so
                        DEPENDS ${THIRD_PARTY_ROOT}/android/skia/out/${BUILD_TYPE}/Shared/libskia.so
                        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/android/skia/out/${BUILD_TYPE}/Shared/libskia.so ${OUTPUT_DIRECTORY}/lib/.
    )
ENDIF()


#######################################################
# GC
#######################################################
SET (GC_CFLAGS_COMMON "-g3 -fPIC -Wno-unused-variable -Wno-unused-function -fdata-sections -ffunction-sections -DESCARGOT -DUSE_GET_STACKBASE_FOR_MAIN -DIGNORE_DYNAMIC_LOADING -DGC_DONT_REGISTER_MAIN_STATIC_DATA")

IF (${CUSTOM} STREQUAL "unified_wearable")
    SET (GC_CFLAGS_CUSTOM "-Os")
ENDIF()

IF (${HOST} STREQUAL "tizen")
    SET (GC_CFLAGS_HOST "-DTIZEN")
ENDIF()

IF (${ARCH} STREQUAL "x86")
    SET (GC_CFLAGS_ARCH "-m32")
    SET (GC_LDFLAGS_ARCH "-m32")
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET (GC_CFLAGS_MODE "-O0")
ELSE()
    SET (GC_CFLAGS_MODE "-O2")
ENDIF()

SET (GC_CFLAGS "${GC_CFLAGS_COMMON} ${GC_CFLAGS_CUSTOM} ${GC_CFLAGS_HOST} ${GC_CFLAGS_ARCH} ${GC_CFLAGS_MODE} $ENV{CFLAGS}")
SET (GC_LDFLAGS "${GC_LDFLAGS_ARCH} ${GC_CFLAGS}")

SET (GC_CONFFLAGS_COMMON --enable-munmap --disable-parallel-mark --enable-large-config --disable-pthread --disable-threads)
IF (${MODE} STREQUAL "debug")
    SET (GC_CONFFLAGS_MODE --enable-debug --enable-gc-debug)
ELSE()
    SET (GC_CONFFLAGS_MODE --disable-debug --disable-gc-debug)
ENDIF()
SET (GC_CONFFLAGS
    ${GC_CONFFLAGS_COMMON}
    ${GC_CONFFLAGS_MODE}
)

SET (GC_BUILDDIR ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared)
IF (${ARCH} STREQUAL "x64")
    SET (GC_TARGET ${GC_BUILDDIR}/.libs/libgc.so)
ELSEIF (${HOST} STREQUAL "tizen")
    SET (GC_TARGET ${GC_BUILDDIR}/.libs/libgc.a)
ELSE()
    MESSAGE (FATAL_ERROR "GC is NOT SUPPORTED")
ENDIF()

ADD_CUSTOM_COMMAND (OUTPUT ${GC_TARGET}
        COMMENT "BUILD GC"
        WORKING_DIRECTORY ${GCUTIL_ROOT}/bdwgc
        COMMAND autoreconf -vif
        COMMAND automake --add-missing
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GC_BUILDDIR}
        COMMAND cd ${GC_BUILDDIR} && ../../../../configure ${GC_CONFFLAGS} CFLAGS=${GC_CFLAGS} LDFLAGS=${GC_LDFLAGS}
        COMMAND cd ${GC_BUILDDIR} && make -j
)

ADD_CUSTOM_TARGET (gc
        DEPENDS ${GC_TARGET}
        COMMAND echo "GC TARGET"
)

#IF (${ARCH} STREQUAL "x64")
#    ADD_LIBRARY (gc SHARED IMPORTED)
#    SET_PROPERTY (TARGET gc PROPERTY
#            IMPORTED_LOCATION ${GC_BUILDDIR}/.libs/libgc.so
#    )
#ELSEIF (${HOST} STREQUAL "tizen")
#    ADD_LIBRARY (gc STATIC IMPORTED)
#    SET_PROPERTY (TARGET gc PROPERTY
#            IMPORTED_LOCATION ${GC_BUILDDIR}/.libs/libgc.a
#    )
#ENDIF()


#######################################################
# ESCARGOT
#######################################################
SET (ESCARGOT_MODE ${MODE})
SET (ESCARGOT_ARCH ${ARCH})
SET (ESCARGOT_OUTPUT static_lib)
IF (${ARCH} STREQUAL "x64")
    SET (ESCARGOT_HOST ${HOST})
ELSE()
    SET (ESCARGOT_HOST tizen_obs)
ENDIF()
ADD_SUBDIRECTORY (third_party/escargot)


#######################################################
# LINK THIRD PARTY LIBRARIES
#######################################################
SET (STARFISH_LIBRARIES_THIRD_PARTY ${GC_TARGET} clipper escargot)

IF (NOT ${CUSTOM} MATCHES "wearable")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} mp4parse webm)
ENDIF()

IF (NOT ${BACKEND} STREQUAL "efl_skia")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} skia_matrix)
ENDIF()

IF (NOT ${HOST} STREQUAL "tizen")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${ZMQ_TARGET})
ENDIF()

IF (${ARCH} STREQUAL "x64" AND (${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "glfw_cairo_gl"))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ELSEIF (${HOST} STREQUAL "tizen" AND (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali"))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()
