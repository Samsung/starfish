CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# THIRD PARTY
#######################################################

# STARFISH THIRDPARTY
#EXECUTE_PROCESS (
#    WORKING_DIRECTORY ${STARFISH_ROOT}
#    COMMAND git submodule init
#    COMMAND git submodule update binding_generator tool/gyp third_party
#)

# ESCARGOT THIRDPARTY
EXECUTE_PROCESS (
    WORKING_DIRECTORY ${ESCARGOT_ROOT}
    COMMAND git submodule init
    COMMAND git submodule update third_party
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
SET (TUV_DEFINITIONS_COMMON
    -DBUILDTESTER=no
    -DBUILD_HOST_HELPER=no
    -DCREATE_SHARED_LIB=yes
    -DTARGET_BOARD=None
)

IF (${MODE} STREQUAL "debug")
    SET (TUV_DEFINITIONS_MODE -DCMAKE_BUILD_TYPE=release)
ELSE()
    SET (TUV_DEFINITIONS_MODE -DCMAKE_BUILD_TYPE=debug)
ENDIF()

IF (${ARCH} STREQUAL "tizen")
    SET (TUV_DEFINITIONS_ARCH -DTARGET_PLATFORM=noarch-tizen)
ELSE()
    SET (TUV_DEFINITIONS_ARCH -DTARGET_PLATFORM=x86_64-linux)
ENDIF()

SET (TUV_DEFINITIONS
    ${TUV_DEFINITIONS_COMMON}
    ${TUV_DEFINITIONS_MODE}
    ${TUV_DEFINITIONS_ARCH}
)

SET (TUV_CFLAGS_COMMON -fno-builtin)
IF (${MODE} STREQUAL "debug")
    SET (TUV_CFLAGS_MODE -O0 -g -DDEBUG)
ELSE()
    SET (TUV_CFLAGS_MODE -O2 -DNDEBUG)
ENDIF()

SET (TUV_CFLAGS
    ${TUV_CFLAGS_COMMON}
    ${TUV_CFLAGS_MODE}
)

SET (TUV_SRC
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/async.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/core.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/fs.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/getaddrinfo.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/loop.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/loop-watcher.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/poll.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/process.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/stream.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/tcp.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/thread.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/timer.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/udp.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/linux-core.c
    ${THIRD_PARTY_ROOT}/libtuv/src/unix/linux-syscalls.c
)

ADD_LIBRARY (tuv SHARED ${TUV_SRC})
TARGET_INCLUDE_DIRECTORIES (tuv PUBLIC ${THIRD_PARTY_ROOT}/libtuv/include ${THIRD_PARTY_ROOT}/libtuv/src)
TARGET_COMPILE_DEFINITIONS (tuv PUBLIC ${TUV_DEFINITIONS})
TARGET_COMPILE_OPTIONS (tuv PUBLIC ${TUV_CFLAGS})

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
SET (ESCARGOT_CXXFLAGS_COMMON "-DESCARGOT -std=c++0x -g3 -fno-math-errno -fdata-sections -ffunction-sections -frounding-math -fsignaling-nans -fno-omit-frame-pointer -fvisibility=hidden -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -Wno-unused-parameter -Wno-type-limits -Wno-unused-result -Wno-unused-variable -Wno-invalid-offsetof -Wno-deprecated-declarations -DESCARGOT_ENABLE_TYPEDARRAY -DESCARGOT_ENABLE_PROMISE -fPIC")
SET (ESCARGOT_LDFLAGS_COMMON "-fvisibility=hidden -Wl,--gc-sections")

IF (${HOST} STREQUAL "linux")
    SET (ESCARGOT_CXXFLAGS_HOST "-fno-rtti -DENABLE_ICU -DENABLE_INTL")
    SET (ESCARGOT_LDFLAGS_HOST "-lpthread -lrt")
ENDIF()

IF (${ARCH} STREQUAL "x64")
    SET (ESCARGOT_CXXFLAGS_ARCH "-DESCARGOT_64=1")
ELSEIF (${ARCH} STREQUAL "x86")
    SET (ESCARGOT_CXXFLAGS_ARCH "-DESCARGOT_32=1")
    IF (NOT ${HOST} STREQUAL "tizen_obs")
        SET (ESCARGOT_CXXFLAGS_ARCH "${ESCARGOT_CXXFLAGS_ARCH} -m32 -mfpmath=sse -msse -msse2")
        SET (ESCARGOT_LDFLAGS_ARCH "-m32")
    ENDIF()
ELSEIF (${ARCH} STREQUAL "arm")
    SET (ESCARGOT_CXXFLAGS_ARCH "-DESCARGOT_32=1")
    IF (NOT ${HOST} STREQUAL "tizen_obs")
        SET (ESCARGOT_CXXFLAGS_ARCH "${ESCARGOT_CXXFLAGS_ARCH} -march=armv7-a -mthumb")
    ENDIF()
ENDIF()

IF (${MODE} STREQUAL "debug")
    SET (ESCARGOT_CXXFLAGS_MODE "-O0 -D_GLIBCXX_DEBUG -Wall -Wextra -Werror -DGC_DEBUG")
ELSE()
    SET (ESCARGOT_CXXFLAGS_MODE "-O2 -DNDEBUG -fno-stack-protector")
ENDIF()

SET (ESCARGOT_CXXFLAGS "${ESCARGOT_CXXFLAGS_COMMON} ${ESCARGOT_CXXFLAGS_HOST} ${ESCARGOT_CXXFLAGS_ARCH} ${ESCARGOT_CXXFLAGS_MODE}")
SET (ESCARGOT_LDFLAGS  "${ESCARGOT_LDFLAGS_COMMON} ${ESCARGOT_LDFLAGS_HOST} ${ESCARGOT_LDFLAGS_ARCH} ${ESCARGOT_LDFLAGS_MODE}")

FILE (GLOB SRC_API_LIST ${ESCARGOT_ROOT}/src/api/*.cpp)
FILE (GLOB SRC_HEAP_LIST ${ESCARGOT_ROOT}/src/heap/*.cpp)
FILE (GLOB SRC_INTERPRETER_LIST ${ESCARGOT_ROOT}/src/interpreter/*.cpp)
FILE (GLOB SRC_PARSER_LIST ${ESCARGOT_ROOT}/src/parser/*.cpp)
FILE (GLOB SRC_PARSER_AST_LIST ${ESCARGOT_ROOT}/src/parser/ast/*.cpp)
FILE (GLOB SRC_PARSER_ESPRIMA_LIST ${ESCARGOT_ROOT}/src/parser/esprima_cpp/*.cpp)
FILE (GLOB SRC_RUNTIME_LIST ${ESCARGOT_ROOT}/src/runtime/*.cpp)
FILE (GLOB SRC_UTIL_LIST ${ESCARGOT_ROOT}/src/util/*.cpp)

FILE (GLOB YARR_LIST ${ESCARGOT_THIRD_PARTY_ROOT}/yarr/*.cpp)
FILE (GLOB DOUBLE_CONVERSION_LIST ${ESCARGOT_THIRD_PARTY_ROOT}/double_conversion/*.cc)
FILE (GLOB GCUTIL_LIST ${GCUTIL_ROOT}/*.cpp)

SET (ESCARGOT_SRC ${SRC_API_LIST} ${SRC_HEAP_LIST} ${SRC_INTERPRETER_LIST} 
     ${SRC_PARSER_LIST} ${SRC_PARSER_AST_LIST} ${SRC_PARSER_ESPRIMA_LIST} 
     ${SRC_RUNTIME_LIST} ${SRC_UTIL_LIST} ${YARR_LIST} 
     ${DOUBLE_CONVERSION_LIST} ${GCUTIL_LIST})

ADD_LIBRARY (escargot STATIC ${ESCARGOT_SRC})
TARGET_INCLUDE_DIRECTORIES (escargot PUBLIC ${ESCARGOT_ROOT}/src/ ${GCUTIL_ROOT}/bdwgc/include/ ${GCUTIL_ROOT}/ ${ESCARGOT_THIRD_PARTY_ROOT}/checked_arithmetic/ ${ESCARGOT_THIRD_PARTY_ROOT}/double_conversion/ ${ESCARGOT_THIRD_PARTY_ROOT}/rapidjson/include/ ${ESCARGOT_THIRD_PARTY_ROOT}/yarr/)
SET_TARGET_PROPERTIES (escargot PROPERTIES 
                       COMPILE_FLAGS "${ESCARGOT_CXXFLAGS}" 
                       LINK_FLAGS "${ESCARGOT_LDFLAGS}"
)
