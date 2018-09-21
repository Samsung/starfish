CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# THIRD PARTY
#######################################################

# ESCARGOT THIRDPARTY
EXECUTE_PROCESS (
    WORKING_DIRECTORY ${ESCARGOT_ROOT}
    COMMAND git submodule update --init third_party
)

# JS BINDING
EXECUTE_PROCESS (
    COMMAND python ${STARFISH_ROOT}/binding_generator/scripts/starfish_code_generator.py ${STARFISH_ROOT}/src/ ${STARFISH_ROOT}/src/binding
)

SET (THIRD_PARTY_CXXFLAGS_COMMON -std=c++11 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-type-limits -fno-math-errno -fdata-sections -ffunction-sections -Wno-invalid-offsetof -fno-omit-frame-pointer -fstack-protector -fPIC)

SET (THIRD_PARTY_CXXFLAGS ${THIRD_PARTY_CXXFLAGS_COMMON} ${LWE_CXXFLAGS_COMPILER} ${LWE_CXXFLAGS_MODE})

# SKIA_MATRIX
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_CORE ${THIRD_PARTY_ROOT}/skia_matrix/src/core/*.cpp)
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_PORTS ${THIRD_PARTY_ROOT}/skia_matrix/src/ports/*.cpp)
ADD_LIBRARY (skia_matrix SHARED ${SKIA_MATRIX_SRC_CORE} ${SKIA_MATRIX_SRC_PORTS})
TARGET_INCLUDE_DIRECTORIES (skia_matrix PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix ${THIRD_PARTY_ROOT}/skia_matrix/include/core ${THIRD_PARTY_ROOT}/skia_matrix/include/private)
TARGET_COMPILE_DEFINITIONS (skia_matrix PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (skia_matrix PUBLIC ${THIRD_PARTY_CXXFLAGS})

# CLIPPER
ADD_LIBRARY (clipper SHARED ${THIRD_PARTY_ROOT}/clipper/cpp/clipper.cpp)
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (clipper PUBLIC ${THIRD_PARTY_CXXFLAGS})

# MP4PARSE
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PUBLIC ${THIRD_PARTY_CXXFLAGS})

# WEBM
ADD_LIBRARY (webm SHARED
    ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
    ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
)
TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)
TARGET_COMPILE_DEFINITIONS (webm PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (webm PUBLIC ${THIRD_PARTY_CXXFLAGS})

# ZERO MQ
SET (ZMQ_CFLAGS_COMMON -g3 -fPIC)
IF (${CUSTOM} STREQUAL "unified_wearable")
    SET (ZMQ_CFLAGS_CUSTOM -Os)
ENDIF()

IF (${ARCH} STREQUAL "x86")
    SET (ZMQ_CFLAGS_ARCH -m32)
ELSEIF (${ARCH} STREQUAL "tizen")
    SET (ZMQ_CFLAGS_ARCH -march=armv7-a -mthumb -finline-limit=64)
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET (ZMQ_CFLAGS_MODE -O0)
ELSE()
    SET (ZMQ_CFLAGS_MODE -O2)
ENDIF()

SET (ZMQ_CFLAGS
    ${ZMQ_CFLAGS_COMMON}
    ${ZMQ_CFLAGS_ARCH}
    ${ZMQ_CFLAGS_MODE}
)

SET (ZMQ_BUILDDIR ${THIRD_PARTY_ROOT}/zeromq/out/${HOST}/${ARCH}/${MODE}.shared) 

ADD_CUSTOM_TARGET (ZMQ_PRECONF
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ZMQ_BUILDDIR}
)

ADD_CUSTOM_TARGET (ZMQ_CONF
        WORKING_DIRECTORY ${ZMQ_BUILDDIR}
        DEPENDS ZMQ_PRECONF
        COMMAND ../../../../configure --enable-static CFLAGS="${ZMQ_CFLAGS}" LDFLAGS="${CFLAGS}" CXXFLAGS="${CFLAGS}"
)

FILE (GLOB_RECURSE ZERO_MQ_SRC ${THIRD_PARTY_ROOT}/zeromq/src/*.cpp)
SET (ZERO_MQ_TWEETNACL ${THIRD_PARTY_ROOT}/zeromq/tweetnacl/src/tweetnacl.c ${THIRD_PARTY_ROOT}/zeromq/tweetnacl/contrib/randombytes/devurandom.c)
ADD_LIBRARY (zmq SHARED ${ZERO_MQ_SRC} ${ZERO_MQ_TWEETNACL})
TARGET_INCLUDE_DIRECTORIES (zmq PUBLIC ${THIRD_PARTY_ROOT}/zeromq/include/ ${THIRD_PARTY_ROOT}/zeromq/tweetnacl/src/ ${THIRD_PARTY_ROOT}/zeromq/tweetnacl/contrib/randombytes/ ${ZMQ_BUILDDIR}/src/)
TARGET_COMPILE_DEFINITIONS (zmq PUBLIC ${LWE_DEFINITIONS})
TARGET_COMPILE_OPTIONS (zmq PUBLIC ${ZMQ_CFLAGS})
ADD_DEPENDENCIES (zmq ZMQ_CONF)

# LIBTUV
INCLUDE (ExternalProject)
SET (LIBTUV_DIR ${THIRD_PARTY_ROOT}/libtuv)

IF (${ARCH} STREQUAL "x64")
    SET (LIBTUV_TARGET_PLATFORM x86_64-linux)
ELSE()
    SET (LIBTUV_TARGET_PLATFORM noarch-tizen)
ENDIF()

SET (LIBTUV_TOOLCHAIN ${LIBTUV_DIR}/cmake/config/config_${LIBTUV_TARGET_PLATFORM}.cmake)

message (${LIBTUV_DIR})
EXTERNALPROJECT_ADD (libtuv
    PREFIX ${LIBTUV_DIR}
    SOURCE_DIR ${LIBTUV_DIR}
    BUILD_IN_SOURCE 0
    BINARY_DIR ${LIBTUV_DIR}
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${LIBTUV_DIR}/build/${LIBTUV_TARGET_PLATFORM}/${MODE}/lib/
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    CMAKE_ARGS
        -B${LIBTUV_DIR}/cmake
        -H./
        -DCMAKE_TOOLCHAIN_FILE=${LIBTUV_TOOLCHAIN}
        -DCMAKE_BUILD_TYPE=${MODE}
        -DTARGET_PLATFORM=${LIBTUV_TARGET_PLATFORM}
        -DLIBTUV_CUSTOM_LIB_OUT=${LIBTUV_DIR}/build/${LIBTUV_TARGET_PLATFORM}/${MODE}/lib
        -DBUILDTESTER=no
        -DBUILD_HOST_HELPER=no
        -DCREATE_SHARED_LIB=yes
        -DTARGET_BOARD=None
)

ADD_LIBRARY (tuv SHARED IMPORTED)
ADD_DEPENDENCIES (tuv libtuv)
SET_PROPERTY (TARGET tuv PROPERTY
    IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)
SET_PROPERTY (DIRECTORY APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)

# LIBSKIA
IF (${HOST} STREQUAL "linux" AND ${BACKEND} STREQUAL "efl_skia")
    SET (BUILD_TYPE "Release")
    IF (${MODE} STREQUAL "debug")
        SET (BUILD_TYPE "Debug")
    ENDIF()
    ADD_CUSTOM_COMMAND (OUTPUT ${OUTPUT_DIRECTORY}/${MODE}/lib
                        DEPENDS ${THIRD_PARTY_ROOT}/android/skia/out/${BUILD_TYPE}/Shared/libskia.so
                        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_ROOT}/android/skia/out/${BUILD_TYPE}/Shared/libskia.so ${OUTPUT_DIRECTORY}/${MODE}/lib
    )
ENDIF()

# GC
SET (GC_DEFINITIONS_COMMON -DHAVE_CONFIG_H -DESCARGOT -DUSE_GET_STACKBASE_FOR_MAIN -DIGNORE_DYNAMIC_LOADING -DGC_DONT_REGISTER_MAIN_STATIC_DATA)
IF (${MODE} STREQUAL "debug")
SET (GC_DEFINITIONS_MODE -DGC_DEBUG)
ENDIF()
SET (GC_DEFINITIONS
    ${LWE_DEFINITIONS}
    ${GC_DEFINITIONS_COMMON}
    ${GC_DEFINITIONS_MODE}
)

SET (GC_CFLAGS_COMMON -g3 -fPIC -Wno-unused-variable -Wno-unused-function -fdata-sections -ffunction-sections)

IF (${CUSTOM} STREQUAL "unified_wearable")
    SET (GC_CFLAGS_CUSTOM -Os)
ENDIF()

IF (${ARCH} STREQUAL "x86")
    SET (GC_CFLAGS_ARCH -m32)
    SET (GC_LDFLAGS_ARCH -m32)
ELSEIF (${ARCH} STREQUAL "tizen")
    SET (GC_CFLAGS_ARCH -march=armv7-a -mthumb -finline-limit=64)
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET (GC_CFLAGS_MODE -O0)
ELSE()
    SET (GC_CFLAGS_MODE -O2)
ENDIF()

SET (GC_CFLAGS
    ${GC_CFLAGS_COMMON}
    ${GC_CFLAGS_ARCH}
    ${GC_CFLAGS_MODE}
)

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

SET (GC_SRC
    ${GCUTIL_ROOT}/bdwgc/allchblk.c
    ${GCUTIL_ROOT}/bdwgc/alloc.c
    ${GCUTIL_ROOT}/bdwgc/backgraph.c
    ${GCUTIL_ROOT}/bdwgc/blacklst.c
    ${GCUTIL_ROOT}/bdwgc/checksums.c
    #${GCUTIL_ROOT}/bdwgc/darwin_stop_world.c
    ${GCUTIL_ROOT}/bdwgc/dbg_mlc.c
    ${GCUTIL_ROOT}/bdwgc/dyn_load.c
    ${GCUTIL_ROOT}/bdwgc/finalize.c
    ${GCUTIL_ROOT}/bdwgc/fnlz_mlc.c
    #${GCUTIL_ROOT}/bdwgc/gc_cpp.cc
    ${GCUTIL_ROOT}/bdwgc/gc_dlopen.c
    ${GCUTIL_ROOT}/bdwgc/gcj_mlc.c
    ${GCUTIL_ROOT}/bdwgc/headers.c
    ${GCUTIL_ROOT}/bdwgc/mach_dep.c
    ${GCUTIL_ROOT}/bdwgc/malloc.c
    ${GCUTIL_ROOT}/bdwgc/mallocx.c
    ${GCUTIL_ROOT}/bdwgc/mark.c
    ${GCUTIL_ROOT}/bdwgc/mark_rts.c
    ${GCUTIL_ROOT}/bdwgc/misc.c
    ${GCUTIL_ROOT}/bdwgc/new_hblk.c
    ${GCUTIL_ROOT}/bdwgc/obj_map.c
    ${GCUTIL_ROOT}/bdwgc/os_dep.c
    #${GCUTIL_ROOT}/bdwgc/pthread_start.c
    #${GCUTIL_ROOT}/bdwgc/pthread_stop_world.c
    #${GCUTIL_ROOT}/bdwgc/pthread_support.c
    ${GCUTIL_ROOT}/bdwgc/pcr_interface.c
    ${GCUTIL_ROOT}/bdwgc/ptr_chck.c
    ${GCUTIL_ROOT}/bdwgc/real_malloc.c
    ${GCUTIL_ROOT}/bdwgc/reclaim.c
    #${GCUTIL_ROOT}/bdwgc/sparc_mach_dep.S
    ${GCUTIL_ROOT}/bdwgc/specific.c
    ${GCUTIL_ROOT}/bdwgc/stubborn.c
    ${GCUTIL_ROOT}/bdwgc/thread_local_alloc.c
    ${GCUTIL_ROOT}/bdwgc/typd_mlc.c 
    #${GCUTIL_ROOT}/bdwgc/win32_threads.c 
    ${GCUTIL_ROOT}/bdwgc/cord/cordbscs.c
    ${GCUTIL_ROOT}/bdwgc/cord/cordprnt.c
    ${GCUTIL_ROOT}/bdwgc/cord/cordxtra.c
)

SET (GC_BUILDDIR ${GCUTIL_ROOT}/bdwgc/out/${HOST}/${ARCH}/${MODE}.shared) 

ADD_CUSTOM_TARGET (GC_PRECONF
        WORKING_DIRECTORY ${GCUTIL_ROOT}/bdwgc
        COMMAND autoreconf -vif
        COMMAND automake --add-missing
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GC_BUILDDIR}
)

ADD_CUSTOM_TARGET (GC_CONF
        WORKING_DIRECTORY ${GC_BUILDDIR}
        DEPENDS GC_PRECONF
        COMMAND ../../../../configure ${GC_CONFFLAGS} CFLAGS="${GC_CFLAGS}" LDFLAGS="${GC_LDFLAGS}"
)

ADD_LIBRARY (gc SHARED ${GC_SRC})
TARGET_INCLUDE_DIRECTORIES (gc PUBLIC ${GCUTIL_ROOT}/bdwgc/include/ ${GC_BUILDDIR}/include/)
TARGET_COMPILE_DEFINITIONS (gc PUBLIC ${GC_DEFINITIONS})
TARGET_COMPILE_OPTIONS (gc PUBLIC ${GC_CFLAGS})
ADD_DEPENDENCIES (gc GC_CONF)


# ESCARGOT
EXTERNALPROJECT_ADD (libescargot
    PREFIX ${ESCARGOT_ROOT}
    SOURCE_DIR ${ESCARGOT_ROOT}
    BUILD_IN_SOURCE 0
    BINARY_DIR ${ESCARGOT_ROOT}
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${ESCARGOT_ROOT}/out/${HOST}/${ARCH}/interpreter/${MODE}/lib/libescargot.a
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    CMAKE_ARGS
        -DHOST=${HOST}
        -DARCH=${ARCH}
        -DMODE=${MODE}
        -DOUTPUT=static_lib
)

ADD_LIBRARY (escargot STATIC IMPORTED)
ADD_DEPENDENCIES (escargot libescargot)
SET_PROPERTY (TARGET escargot PROPERTY
    IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libescargot.a)
SET_PROPERTY (DIRECTORY APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libescargot.a)
