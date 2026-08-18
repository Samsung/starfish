CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# Use CMAKE_C_COMPILER and CMAKE_CXX_COMPILER directly
# These can be set via -DCMAKE_C_COMPILER=xxx or -DCMAKE_CXX_COMPILER=xxx
# If not explicitly set, CMake uses CC/CXX env vars or auto-detects
IF (CMAKE_C_COMPILER)
    SET (THIRD_PARTY_C_COMPILER_OPTION ${CMAKE_C_COMPILER})
ELSEIF (${CMAKE_CXX_COMPILER_ID} MATCHES  "GNU")
    SET (THIRD_PARTY_C_COMPILER_OPTION "gcc")
ELSEIF (${CMAKE_CXX_COMPILER_ID} MATCHES  "Clang")
    SET (THIRD_PARTY_C_COMPILER_OPTION "clang")
ENDIF()

IF (CMAKE_CXX_COMPILER)
    SET (THIRD_PARTY_CXX_COMPILER_OPTION ${CMAKE_CXX_COMPILER})
ELSEIF (${CMAKE_CXX_COMPILER_ID} MATCHES  "GNU")
    SET (THIRD_PARTY_CXX_COMPILER_OPTION "g++")
ELSEIF (${CMAKE_CXX_COMPILER_ID} MATCHES  "Clang")
    SET (THIRD_PARTY_CXX_COMPILER_OPTION "clang++")
ENDIF()

# Lowercase mirrors of CMAKE_SYSTEM_NAME/CMAKE_BUILD_TYPE/CMAKE_SYSTEM_PROCESSOR
# for sub-build path/argument interpolation (e.g. nanomsg/libwebsockets/openssl/
# webrtc/tuv build directories and TUV_BUILD_TYPE=) that expect the old
# lowercase HOST/MODE/ARCH spellings ("linux", "debug", "x64", ...).
STRING (TOLOWER "${CMAKE_SYSTEM_NAME}" HOST_LOWER)
STRING (TOLOWER "${CMAKE_BUILD_TYPE}" MODE_LOWER)
IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET (ARCH_LOWER "x64")
ELSE()
    SET (ARCH_LOWER "${CMAKE_SYSTEM_PROCESSOR}")
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
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_CORE ${THIRD_PARTY_ROOT}/skia_matrix/src/core/*.cpp)
FILE (GLOB_RECURSE SKIA_MATRIX_SRC_PORTS ${THIRD_PARTY_ROOT}/skia_matrix/src/ports/*.cpp)
ADD_LIBRARY (skia_matrix SHARED ${SKIA_MATRIX_SRC_CORE} ${SKIA_MATRIX_SRC_PORTS})
TARGET_INCLUDE_DIRECTORIES (skia_matrix PUBLIC ${THIRD_PARTY_ROOT}/skia_matrix ${THIRD_PARTY_ROOT}/skia_matrix/include/core ${THIRD_PARTY_ROOT}/skia_matrix/include/private)
TARGET_COMPILE_DEFINITIONS (skia_matrix PUBLIC ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (skia_matrix PUBLIC ${THIRD_PARTY_CXXFLAGS})
# Explicit bare SONAME -- without it, whether this library ends up with any
# embedded soname at all depends on CMake finding a full SONAME-flag
# definition for the active CMAKE_SYSTEM_NAME's platform module. That held
# for "Linux" (used, incorrectly, even for actual Tizen cross builds before
# CMAKE_SYSTEM_NAME=Tizen was passed explicitly) but not for "Tizen" itself,
# whose platform module doesn't define it: no soname got embedded, so any
# consumer linking against this by its build-tree path got that raw path
# baked into its own DT_NEEDED instead of a bare filename -- resolved fine
# in the build tree, "not found" once installed to a different layout
# (confirmed via `ldd liblightweight-web-engine.mobile-impl.so` on-device:
# "lib/libskia_matrix.so => not found" -- a relative path with a slash in
# it, which the dynamic linker never searches RPATH for at all).
SET_TARGET_PROPERTIES (skia_matrix PROPERTIES LINK_FLAGS "-Wl,-soname,libskia_matrix.so")


#######################################################
# CLIPPER
#######################################################
FILE (GLOB CLIPPER_SRC ${THIRD_PARTY_ROOT}/clipper/cpp/*.cpp)
ADD_LIBRARY (clipper SHARED ${CLIPPER_SRC})
TARGET_INCLUDE_DIRECTORIES (clipper PUBLIC ${THIRD_PARTY_ROOT}/clipper/cpp/)
TARGET_COMPILE_DEFINITIONS (clipper PUBLIC ${THIRD_PARTY_DEFINITIONS})
IF (${CMAKE_CXX_COMPILER_ID} MATCHES  "GNU" OR ${CMAKE_CXX_COMPILER_ID} MATCHES  "Clang")
    TARGET_COMPILE_OPTIONS (clipper PUBLIC ${THIRD_PARTY_CXXFLAGS} -fvisibility=hidden)
ELSE()
    TARGET_COMPILE_OPTIONS (clipper PUBLIC ${THIRD_PARTY_CXXFLAGS})
ENDIF()
# See the comment on skia_matrix's SET_TARGET_PROPERTIES above.
SET_TARGET_PROPERTIES (clipper PROPERTIES LINK_FLAGS "-Wl,-soname,libclipper.so")

#######################################################
# MP4PARSE
#######################################################
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PUBLIC ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PUBLIC ${THIRD_PARTY_CXXFLAGS})
# See the comment on skia_matrix's SET_TARGET_PROPERTIES above.
SET_TARGET_PROPERTIES (mp4parse PROPERTIES LINK_FLAGS "-Wl,-soname,libmp4parse.so")


#######################################################
# WEBM
#######################################################
ADD_LIBRARY (webm SHARED
    ${THIRD_PARTY_ROOT}/webm/mkvparser/mkvparser.cc
    ${THIRD_PARTY_ROOT}/webm/webvtt/webvttparser.cc
)
TARGET_INCLUDE_DIRECTORIES (webm PUBLIC ${THIRD_PARTY_ROOT}/webm/)
TARGET_COMPILE_DEFINITIONS (webm PUBLIC ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (webm PUBLIC ${THIRD_PARTY_CXXFLAGS})
# See the comment on skia_matrix's SET_TARGET_PROPERTIES above.
SET_TARGET_PROPERTIES (webm PROPERTIES LINK_FLAGS "-Wl,-soname,libwebm.so")


#######################################################
# NANOMSG
#######################################################
# Nanomsg is used for SharedWorker, ServiceWorker and Inspector
IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")
    SET (NANOMSG_BUILDDIR ${OUTPUT_DIRECTORY}/nanomsg/out/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER}.shared)
    SET (NANOMSG_LOCAL_TARGET ${NANOMSG_BUILDDIR}/libnanomsg.so)
    SET (NANOMSG_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libnanomsg.so)

    SET (NANOMSG_CFLAGS_COMMON "-g3 -fPIC")
    IF (${CUSTOM} STREQUAL "unified_wearable")
        SET (NANOMSG_CFLAGS_CUSTOM "-Os")
    ENDIF()

    IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
        SET (NANOMSG_CFLAGS_ARCH "-m32")
    ELSEIF (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
        SET (NANOMSG_CFLAGS_ARCH "-march=armv7-a -mthumb -finline-limit=64")
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        SET (NANOMSG_CFLAGS_MODE "-O0")
    ELSE()
        SET (NANOMSG_CFLAGS_MODE "-O2")
    ENDIF()

    SET (NANOMSG_CFLAGS "${NANOMSG_CFLAGS_COMMON} ${NANOMSG_CFLAGS_CUSTOM} ${NANOMSG_CFLAGS_ARCH} ${NANOMSG_CFLAGS_MODE}")
    SET (NANOMSG_CUSTOM -DNN_ENABLE_DOC=OFF -DNN_TESTS=OFF -DNN_TOOLS=OFF -DNN_ENABLE_GETADDRINFO_A=OFF -DCMAKE_INSTALL_PREFIX=${NANOMSG_BUILDDIR}/dist)

    ADD_CUSTOM_COMMAND (OUTPUT ${NANOMSG_LOCAL_TARGET}
                        COMMENT "BUILD NANOMSG"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${NANOMSG_BUILDDIR}
                        COMMAND cd ${THIRD_PARTY_ROOT}/nanomsg/ && cmake ${CMAKE_COMMAND} -S . -B${NANOMSG_BUILDDIR} -DCMAKE_C_FLAGS=${NANOMSG_CFLAGS} -DCMAKE_CXX_FLAGS=${NANOMSG_CFLAGS} ${NANOMSG_CUSTOM} -DCMAKE_C_COMPILER=${THIRD_PARTY_C_COMPILER_OPTION} -DCMAKE_CXX_COMPILER=${THIRD_PARTY_CXX_COMPILER_OPTION}
                        COMMAND cd ${THIRD_PARTY_ROOT}/nanomsg/ && cmake --build ${NANOMSG_BUILDDIR}
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${NANOMSG_TARGET}
                        DEPENDS ${NANOMSG_LOCAL_TARGET}
                        COMMENT "COPY NANOMSG"
                        COMMAND cp -P ${NANOMSG_LOCAL_TARGET}* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
                        COMMENT "INSTALL NANOMSG"
                        COMMAND cd ${NANOMSG_BUILDDIR} && make install
    )

    ADD_CUSTOM_TARGET (nanomsg
                       DEPENDS ${NANOMSG_TARGET}
                       COMMENT "NANOMSG TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${NANOMSG_BUILDDIR}/dist/include)
ENDIF()

#######################################################
# LIBWEBSOCKETS
#######################################################

IF (${USE_LIBWEBSOCKETS} STREQUAL "1")
    SET(LIBWEBSOCKETS_SOURCE_DIR ${THIRD_PARTY_ROOT}/libwebsockets/)
    SET(LIBWEBSOCKETS_BUILD_DIR ${OUTPUT_DIRECTORY}/libwebsockets/)
    SET(LIBWEBSOCKETS_BUILD_OUTDIR ${OUTPUT_DIRECTORY}/libwebsockets/build/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER})
    SET(LIBWEBSOCKETS_LOCAL_TARGET ${LIBWEBSOCKETS_BUILD_OUTDIR}/lib/libwebsockets_lwe.so)
    SET(LIBWEBSOCKETS_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebsockets_lwe.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
                        COMMENT "COPY LIBWEBSOCKETS SOURCE"
                        COMMAND cp -r ${LIBWEBSOCKETS_SOURCE_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND sed -i "s/hidden/default/" ${LIBWEBSOCKETS_BUILD_DIR}/include/libwebsockets.h
                        COMMAND touch ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
    )

    # Forward cross-compile settings to the libwebsockets sub-build so its
    # find_library()/find_path() resolve against the target sysroot (e.g. libcap)
    # instead of the host. Without this, cross builds abort with
    # "LIBCAP_LIBRARIES ... set to NOTFOUND" during the TLS feature checks.
    SET (LIBWEBSOCKETS_CROSS_OPTION "")
    IF (CMAKE_CROSSCOMPILING)
        SET (LIBWEBSOCKETS_CROSS_OPTION
            -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
            -DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
            -DCMAKE_SYSROOT=${CMAKE_SYSROOT}
            -DCMAKE_FIND_ROOT_PATH=${CMAKE_SYSROOT}
            -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
            -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
            -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY)
    ENDIF()

    IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	    SET (OPENSSL_LIB_CUSTOM "-DLWS_OPENSSL_LIBRARIES=\"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.so\"")
        SET (OPENSSL_BUILD_PATH ${OUTPUT_DIRECTORY}/openssl/out/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER})
	SET (LIBWEBSOCKETS_BUILD_OPTION -DSTARFISH_CUSTOM=1 -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF -DLWS_HAVE_VISIBILITY:BOOL=ON -DLWS_STATIC_PIC:BOOL=OFF -DOPENSSL_ROOT_DIR=${OPENSSL_BUILD_PATH}/source -DLWS_OPENSSL_INCLUDE_DIRS=${OPENSSL_BUILD_PATH}/source/include -DLWS_WITH_SSL=1 -DLWS_WITH_TLS=1)
        ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
                            DEPENDS openssl ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
                            WORKING_DIRECTORY ${LIBWEBSOCKETS_BUILD_DIR}
                            COMMENT "BUILD LIBWEBSOCKETS"
                            COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_OUTDIR}
                            COMMAND test -f ${LIBWEBSOCKETS_LOCAL_TARGET} || ${CMAKE_COMMAND} -S . -B${LIBWEBSOCKETS_BUILD_OUTDIR} -G Ninja ${LIBWEBSOCKETS_BUILD_OPTION} ${LIBWEBSOCKETS_CROSS_OPTION} -DCMAKE_C_COMPILER=${THIRD_PARTY_C_COMPILER_OPTION} -DCMAKE_CXX_COMPILER=${THIRD_PARTY_CXX_COMPILER_OPTION} "${OPENSSL_LIB_CUSTOM}"
                            COMMAND test -f ${LIBWEBSOCKETS_LOCAL_TARGET} || ${CMAKE_COMMAND} --build ${LIBWEBSOCKETS_BUILD_OUTDIR}
                            COMMAND touch ${LIBWEBSOCKETS_LOCAL_TARGET}
        )
    ELSE()
        SET (LIBWEBSOCKETS_BUILD_OPTION -DSTARFISH_CUSTOM=1 -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF -DLWS_HAVE_VISIBILITY:BOOL=ON -DLWS_STATIC_PIC:BOOL=OFF)
        # Pin the sub-build to the very OpenSSL the engine itself uses
        # (openssl1.1 up to Tizen 10.0, openssl3 from Tizen 10.1 on) instead of
        # letting its find_package() pick whatever it stumbles upon. The library
        # list has to stay one single argument, hence the quoting used below.
        SET (OPENSSL_LIB_CUSTOM "")
        IF (DEFINED STARFISH_OPENSSL_MODULE)
            SET (LIBWEBSOCKETS_BUILD_OPTION ${LIBWEBSOCKETS_BUILD_OPTION} -DLWS_WITH_SSL=1 -DLWS_WITH_TLS=1 -DLWS_OPENSSL_INCLUDE_DIRS=${STARFISH_OPENSSL_INCLUDEDIR})
            SET (OPENSSL_LIB_CUSTOM "-DLWS_OPENSSL_LIBRARIES=\"${STARFISH_OPENSSL_LIBDIR}/libssl.so;${STARFISH_OPENSSL_LIBDIR}/libcrypto.so\"")
        ENDIF()
        ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
                            DEPENDS ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
                            WORKING_DIRECTORY ${LIBWEBSOCKETS_BUILD_DIR}
                            COMMENT "BUILD LIBWEBSOCKETS"
                            COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_OUTDIR}
                            COMMAND test -f ${LIBWEBSOCKETS_LOCAL_TARGET} || ${CMAKE_COMMAND} -S . -B${LIBWEBSOCKETS_BUILD_OUTDIR} -G Ninja ${LIBWEBSOCKETS_BUILD_OPTION} ${LIBWEBSOCKETS_CROSS_OPTION} -DCMAKE_C_COMPILER=${THIRD_PARTY_C_COMPILER_OPTION} -DCMAKE_CXX_COMPILER=${THIRD_PARTY_CXX_COMPILER_OPTION} "${OPENSSL_LIB_CUSTOM}"
                            COMMAND test -f ${LIBWEBSOCKETS_LOCAL_TARGET} || ${CMAKE_COMMAND} --build ${LIBWEBSOCKETS_BUILD_OUTDIR}
                            COMMAND touch ${LIBWEBSOCKETS_LOCAL_TARGET}
        )
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_TARGET}
                        DEPENDS ${LIBWEBSOCKETS_LOCAL_TARGET}
                        COMMENT "COPY LIBWEBSOCKETS"
                        COMMAND cp ${LIBWEBSOCKETS_LOCAL_TARGET} ${LIBWEBSOCKETS_TARGET}
                        # COMMAND patchelf --set-soname libwebsockets_lwe.so ${LIBWEBSOCKETS_TARGET}
    )

    ADD_CUSTOM_TARGET (libwebsockets
                        DEPENDS ${LIBWEBSOCKETS_TARGET}
                        COMMENT "LIBWEBSOCKETS TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${LIBWEBSOCKETS_BUILD_OUTDIR}/include ${LIBWEBSOCKETS_SOURCE_DIR}/include)
ENDIF()

#######################################################
# LIBTUV
#######################################################
IF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND
        (${BACKEND} STREQUAL "uv_cairo_gl" OR ${WORKER} STREQUAL "1" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")))
    SET (TUV_DIR ${THIRD_PARTY_ROOT}/libtuv)
    SET (TUV_BUILD_DIR ${OUTPUT_DIRECTORY}/libtuv)
    SET (TUV_LOCAL_TARGET ${TUV_BUILD_DIR}/build/x86_64-linux/${MODE_LOWER}/lib/libtuv.so)
    SET (TUV_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_LOCAL_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        # we should copy tuv repo because tuv make include file inside of tuv repo.
                        COMMAND cp -r ${TUV_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${TUV_BUILD_DIR} && make -j TUV_BUILD_TYPE=${MODE_LOWER} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=x86_64-linux
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        DEPENDS ${TUV_LOCAL_TARGET}
                        COMMENT "COPY TUV"
                        COMMAND cp ${TUV_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (tuv
                       DEPENDS ${TUV_TARGET}
                       COMMENT "TUV TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${TUV_BUILD_DIR}/src ${TUV_BUILD_DIR}/include)
ELSEIF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (CMAKE_SYSTEM_NAME STREQUAL "Tizen" AND (${BACKEND} STREQUAL "flutter" OR ${BACKEND} STREQUAL "uv_cairo_gl"
        OR ${WORKER} STREQUAL "1" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")))
    SET (TUV_DIR ${THIRD_PARTY_ROOT}/libtuv)
    SET (TUV_BUILD_DIR ${OUTPUT_DIRECTORY}/libtuv)
    SET (TUV_LOCAL_TARGET ${TUV_BUILD_DIR}/build/noarch-tizen/${MODE_LOWER}/lib/libtuv.so)
    SET (TUV_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_LOCAL_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        # we should copy tuv repo because tuv make include file inside of tuv repo.
                        COMMAND cp -r ${TUV_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cp ${TUV_DIR}/config/tizen/packaging/libtuv.pc.in ${TUV_BUILD_DIR}
                        COMMAND cd ${TUV_BUILD_DIR} && make -j TUV_BUILD_TYPE=${MODE_LOWER} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=noarch-tizen
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        DEPENDS ${TUV_LOCAL_TARGET}
                        COMMENT "COPY TUV"
                        COMMAND cp ${TUV_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (tuv
                       DEPENDS ${TUV_TARGET}
                       COMMENT "TUV TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${TUV_BUILD_DIR}/src ${TUV_BUILD_DIR}/include)
ENDIF()

#######################################################
# LIBCAIRO
#######################################################
IF (${BUILD_CAIRO} STREQUAL "1")
    SET (CAIRO_DIR ${THIRD_PARTY_ROOT}/cairo)
    SET (CAIRO_TARGET ${OUTPUT_DIRECTORY}/cairo/out/lib/libcairo.a)

    ADD_CUSTOM_COMMAND (OUTPUT ${CAIRO_TARGET}
                        WORKING_DIRECTORY ${CAIRO_DIR}
                        COMMENT "BUILD CAIRO"
                        COMMAND NOCONFIGURE=1 ./autogen.sh
                        COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ./configure --prefix=${OUTPUT_DIRECTORY}/cairo/out --with-pic --enable-fc --enable-ft --enable-tee --disable-xlib --disable-xcb --disable-gtk-doc --enable-static
                        COMMAND make -j${NPROCS} V=1
                        COMMAND make install
                        COMMAND make distclean
                        COMMAND rm -f build/gtk-doc.m4
                        COMMAND rm -f configure gtk-doc.make aclocal.m4
    )

    ADD_CUSTOM_TARGET (own_cairo
                       DEPENDS ${CAIRO_TARGET}
                       COMMENT "CAIRO TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${OUTPUT_DIRECTORY}/cairo/out/include ${OUTPUT_DIRECTORY}/cairo/out/include/cairo)
ENDIF()

#######################################################
# ESCARGOT
#######################################################
IF (${ENABLE_WASM} STREQUAL "1")
    SET (ESCARGOT_WASM ON)
ENDIF()
IF (${ENABLE_CODECACHE} STREQUAL "1")
    SET (ESCARGOT_CODE_CACHE ON)
ENDIF()

IF (NOT DEFINED ESCARGOT_HOST)
    IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        SET (ESCARGOT_HOST linux)
    ELSE()
        SET (ESCARGOT_HOST tizen_obs)
    ENDIF()
ENDIF()

IF (${ENABLE_DEBUGGER} STREQUAL "1")
    SET (ESCARGOT_DEBUGGER ON)
ENDIF()

SET (ESCARGOT_USE_CUSTOM_LOGGING ON)

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (ESCARGOT_CXXFLAGS_FROM_EXTERNAL ${LWE_CXXFLAGS_FORCE_NOLTO})
    SET (ESCARGOT_CFLAGS_FROM_EXTERNAL ${LWE_CFLAGS_FORCE_NOLTO})
    SET (ESCARGOT_LDFLAGS_FROM_EXTERNAL ${LWE_LDFLAGS_FORCE_NOLTO})
ENDIF()

IF (${STARFISH_ENABLE_THREADING})
    SET (ESCARGOT_THREADING ON)
    # (No add_compile_options(-DGC_THREAD_ISOLATE=1) here: it was a global leak
    # into every target in this directory scope -- starfish's own code, gtest,
    # every third-party lib -- for a macro that only third_party/escargot's
    # own GCutil headers ever check (#if defined(GC_THREAD_ISOLATE)). escargot
    # already gets it correctly: ESCARGOT_THREADING=ON above flows into
    # escargot's own GCUTIL_ENABLE_THREADING, which GCutil's CMakeLists uses
    # to add -DGC_THREAD_ISOLATE=1 to its own target scope.)
    # BY_ADDRESS assumes every thread's GC TLS variable sits at the same
    # fixed offset from the thread pointer, which local-dynamic TLS doesn't
    # guarantee -- confirmed crashing a worker thread's GC_init ("there is
    # a error calc tls offset", GCutil/misc.c). Use PTHREAD_KEY instead,
    # which doesn't depend on TLS layout at all; no need to force a TLS
    # model anymore either. ENABLE_TLS_ACCESS_BY_PTHREAD_KEY defaults ON but
    # packaging turns it off for tizen_version_major <= 8 (untested there);
    # off just falls back to GCutil's plain thread_local path, no crash risk
    # either way.
    IF (${ENABLE_TLS_ACCESS_BY_PTHREAD_KEY})
        SET (ESCARGOT_TLS_ACCESS_BY_PTHREAD_KEY ON)
    ENDIF()
ENDIF()

ADD_SUBDIRECTORY (third_party/escargot)

#######################################################
# OpenSSL
#######################################################
# Used when a target platform does not have openssl.
# Build in separate directory to avoid conflicts between multiple build configs
IF (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET (OPENSSL_DIR ${THIRD_PARTY_ROOT}/openssl)
    SET (OPENSSL_BUILD_PATH ${OUTPUT_DIRECTORY}/openssl/out/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER})
    SET (OPENSSL_LOCAL_TARGET ${OPENSSL_BUILD_PATH}/libssl.so)
    SET (OPENSSL_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so)

    IF (CMAKE_CROSSCOMPILING)
        # Map the build ARCH to an OpenSSL Configure target triplet so the
        # cross build works for every cross arch (not just aarch64).
        IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
            SET (OPENSSL_CONFIGURE_TARGET linux-aarch64)
        ELSEIF (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
            SET (OPENSSL_CONFIGURE_TARGET linux-armv4)
        ELSEIF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
            SET (OPENSSL_CONFIGURE_TARGET linux-x86)
        ELSE()
            SET (OPENSSL_CONFIGURE_TARGET linux-x86_64)
        ENDIF()

        # Pass the sysroot so the cross toolchain resolves target headers/libs
        # (e.g. glib) instead of host ones. OpenSSL's Configure has no dedicated
        # --sysroot option but passes through unrecognized -/-- args as compiler
        # flags, and we also export it via CFLAGS/CXXFLAGS/LDFLAGS for the link step.
        IF (CMAKE_SYSROOT)
            SET (OPENSSL_SYSROOT_OPTION --sysroot=${CMAKE_SYSROOT})
        ELSE()
            SET (OPENSSL_SYSROOT_OPTION "")
        ENDIF()

        SET (OPENSSL_CONFIGURE_CMD env "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}" "CFLAGS=$ENV{CFLAGS}" "CXXFLAGS=$ENV{CXXFLAGS}" "LDFLAGS=$ENV{LDFLAGS}" ./Configure ${OPENSSL_CONFIGURE_TARGET} --prefix=${OPENSSL_BUILD_PATH} ${OPENSSL_SYSROOT_OPTION} no-asm shared)
    ELSE()
        SET (OPENSSL_CONFIGURE_CMD ./config --prefix=${OPENSSL_BUILD_PATH})
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_LOCAL_TARGET}
                        WORKING_DIRECTORY ${OPENSSL_BUILD_PATH}
                        COMMENT "BUILDING OPENSSL"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${OPENSSL_BUILD_PATH}
                        COMMAND test -f ${OPENSSL_LOCAL_TARGET} || cp -r ${OPENSSL_DIR} ${OPENSSL_BUILD_PATH}/source
                        COMMAND test -f ${OPENSSL_LOCAL_TARGET} || cd ${OPENSSL_BUILD_PATH}/source
                        COMMAND test -f ${OPENSSL_LOCAL_TARGET} || ${OPENSSL_CONFIGURE_CMD}
                        COMMAND test -f ${OPENSSL_LOCAL_TARGET} || make -j8 build_generated
                        COMMAND test -f ${OPENSSL_LOCAL_TARGET} || make -j8 build_libs
                        COMMAND test -f ${OPENSSL_LOCAL_TARGET} || touch ${OPENSSL_LOCAL_TARGET}
                        DEPENDS ${OPENSSL_DIR}/config
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_TARGET}
                        WORKING_DIRECTORY ${OPENSSL_DIR}
                        DEPENDS ${OPENSSL_LOCAL_TARGET}
                        COMMENT "COPYING OPENSSL"
                        COMMAND cp -P ${OPENSSL_BUILD_PATH}/source/lib*so* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (openssl
                    DEPENDS ${OPENSSL_TARGET}
                    COMMENT "OPENSSL TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${OPENSSL_BUILD_PATH}/source/include)
ENDIF()

#######################################################
# WEBRTC
#######################################################

SET (WEBRTC_DIR ${THIRD_PARTY_ROOT}/webrtc/src)

IF (${WEBRTC} STREQUAL "1")
    EXECUTE_PROCESS (
        WORKING_DIRECTORY ${STARFISH_ROOT}/third_party/webrtc
        COMMAND git submodule update --init
    )
    SET(WEBRTC_BUILD_PATH libwebrtc/libs/${HOST_LOWER}/${ARCH_LOWER}/${MODE_LOWER})
    SET(WEBRTC_LOCAL_TARGET ${WEBRTC_DIR}/${WEBRTC_BUILD_PATH}/libwebrtc.so)
    SET(WEBRTC_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebrtc.so)
    ADD_CUSTOM_COMMAND (OUTPUT ${WEBRTC_TARGET}
                        WORKING_DIRECTORY ${WEBRTC_DIR}
                        DEPENDS ${WEBRTC_LOCAL_TARGET}
                        COMMENT "COPY WEBRTC"
                        COMMAND cp ${WEBRTC_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )
    ADD_CUSTOM_TARGET (webrtc
                       DEPENDS ${WEBRTC_TARGET}
                       COMMENT "WEBRTC TARGET"
    )
ENDIF()

#######################################################
# LIBPNG
#######################################################
IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
    SET (PNG_DIR ${THIRD_PARTY_ROOT}/libpng)
    SET (PNG_BUILD_DIR ${OUTPUT_DIRECTORY}/libpng/)
    SET (PNG_LOCAL_TARGET ${OUTPUT_DIRECTORY}/libpng/libpng16.so)
    SET (PNG_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpng_lwe.so)
    SET (PNG_OPTION "-DPNG_STATIC=OFF -DSKIP_INSTALL_PROGRAMS=ON -DSKIP_INSTALL_EXPORT=ON")

    IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
        SET (PNG_OPTION ${PNG_OPTION}" -D_ARCH_ARM_ -mfpu=neon -DPNG_ARM_NEON=check")
    ELSEIF(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        SET (PNG_OPTION ${PNG_OPTION}" -D_ARCH_ARM_ -mfpu=neon -DPNG_ARM_NEON=on")
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${PNG_LOCAL_TARGET}
                        WORKING_DIRECTORY ${PNG_DIR}
                        COMMENT "BUILD PNG"
                        COMMAND cp -r ${PNG_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${PNG_BUILD_DIR} && CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ${CMAKE_COMMAND} ${PNG_OPTION} -DCMAKE_C_COMPILER=${THIRD_PARTY_C_COMPILER_OPTION} -DCMAKE_CXX_COMPILER=${THIRD_PARTY_CXX_COMPILER_OPTION}
                        COMMAND cd ${PNG_BUILD_DIR} && ${CMAKE_COMMAND} --build .
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${PNG_TARGET}
                        WORKING_DIRECTORY ${PNG_BUILD_DIR}
                        DEPENDS ${PNG_LOCAL_TARGET}
                        COMMENT "COPY AND PATCH PNG"
                        COMMAND cp ${PNG_LOCAL_TARGET} ${PNG_TARGET}
                        COMMAND patchelf --set-soname libpng_lwe.so ${PNG_TARGET}
    )

    ADD_CUSTOM_TARGET (libpng
                       DEPENDS ${PNG_TARGET}
                       COMMENT "PNG TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${PNG_BUILD_DIR}/)
ENDIF()

#######################################################
# GIFLIB
#######################################################
IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
    SET (GIF_DIR ${THIRD_PARTY_ROOT}/giflib)
    SET (GIF_BUILD_DIR ${OUTPUT_DIRECTORY}/giflib/)
    SET (GIF_LOCAL_TARGET ${OUTPUT_DIRECTORY}/giflib/libgif.so)
    SET (GIF_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libgif_lwe.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${GIF_LOCAL_TARGET}
                        WORKING_DIRECTORY ${GIF_DIR}
                        COMMENT "BUILD GIF"
                        COMMAND rm -rf ${GIF_BUILD_DIR}
                        COMMAND cp -r ${GIF_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${GIF_BUILD_DIR} && CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} make libgif.so
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${GIF_TARGET}
                        WORKING_DIRECTORY ${GIF_BUILD_DIR}
                        DEPENDS ${GIF_LOCAL_TARGET}
                        COMMENT "COPY AND PATCH GIF"
                        COMMAND cp ${GIF_LOCAL_TARGET} ${GIF_TARGET}
                        COMMAND patchelf --set-soname libgif_lwe.so ${GIF_TARGET}
    )

    ADD_CUSTOM_TARGET (giflib
                       DEPENDS ${GIF_TARGET}
                       COMMENT "GIF TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${GIF_BUILD_DIR}/)
ENDIF()

#######################################################
# JPEG
#######################################################
IF (${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
    SET (JPEG_DIR ${THIRD_PARTY_ROOT}/libjpeg-turbo)
    SET (JPEG_BUILD_DIR ${OUTPUT_DIRECTORY}/libjpeg-turbo/)
    SET (JPEG_LOCAL_TARGET ${OUTPUT_DIRECTORY}/libjpeg-turbo/libjpeg.so)
    SET (JPEG_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libjpeg_lwe.so)
    SET (JPEG_OPTION "-DCMAKE_BUILD_TYPE=Release -DENABLE_SHARED=TRUE -DENABLE_STATIC=FALSE -DWITH_JPEG8=TRUE")

    IF(CMAKE_SYSTEM_NAME STREQUAL "Tizen" AND ${CUSTOM} STREQUAL "prod_tv")
        SET (JPEG_OPTION ${JPEG_OPTION}" -DENABLE_COLOR_PICKER=TRUE -DCMAKE_C_FLAGS='-D_TIZEN_PRODUCT_TV -D_USE_PRODUCT_TV'")
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${JPEG_LOCAL_TARGET}
                        WORKING_DIRECTORY ${JPEG_DIR}
                        COMMENT "BUILD PNG"
                        COMMAND rm -rf ${JPEG_BUILD_DIR}
                        COMMAND cp -r ${JPEG_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${JPEG_BUILD_DIR} && CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ${CMAKE_COMMAND} ${JPEG_OPTION} -DCMAKE_C_COMPILER=${THIRD_PARTY_C_COMPILER_OPTION} -DCMAKE_CXX_COMPILER=${THIRD_PARTY_CXX_COMPILER_OPTION}
                        COMMAND cd ${JPEG_BUILD_DIR} && ${CMAKE_COMMAND} --build .
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${JPEG_TARGET}
                        WORKING_DIRECTORY ${JPEG_BUILD_DIR}
                        DEPENDS ${JPEG_LOCAL_TARGET}
                        COMMENT "COPY AND PATCH JPEG"
                        COMMAND cp ${JPEG_LOCAL_TARGET} ${JPEG_TARGET}
                        COMMAND patchelf --set-soname libjpeg_lwe.so ${JPEG_TARGET}
    )

    ADD_CUSTOM_TARGET (turbojpeg
                       DEPENDS ${JPEG_TARGET}
                       COMMENT "JPEG TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${JPEG_BUILD_DIR}/)
ENDIF()

#######################################################
# LIBWEBP
#######################################################
IF (${USE_CUSTOM_WEBP} STREQUAL "1" OR ${USE_EMBEDDED_IMAGE_DECODER} STREQUAL "1")
    SET (WEBP_DIR ${THIRD_PARTY_ROOT}/libwebp)
    SET (WEBP_BUILD_DIR ${OUTPUT_DIRECTORY}/libwebp/)
    SET (WEBP_LOCAL_TARGET ${OUTPUT_DIRECTORY}/libwebp/libwebp.so)
    SET (WEBP_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebp_lwe.so)
    SET (WEBP_OPTION "-DBUILD_SHARED_LIBS=TRUE")
IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen")
    SET (WEBP_BUILD_OPTION "-D__TIZEN__")
ELSE()
    SET (WEBP_BUILD_OPTION "")
ENDIF()
    ADD_CUSTOM_COMMAND (OUTPUT ${WEBP_LOCAL_TARGET}
                        WORKING_DIRECTORY ${WEBP_DIR}
                        COMMENT "BUILD WEBP"
                        COMMAND cp -r ${WEBP_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${WEBP_BUILD_DIR} && CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=${WEBP_BUILD_OPTION} ${CMAKE_COMMAND} ${WEBP_OPTION} -DCMAKE_C_COMPILER=${THIRD_PARTY_C_COMPILER_OPTION} -DCMAKE_CXX_COMPILER=${THIRD_PARTY_CXX_COMPILER_OPTION}
                        COMMAND cd ${WEBP_BUILD_DIR} && ${CMAKE_COMMAND} --build .
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${WEBP_TARGET}
                        WORKING_DIRECTORY ${WEBP_BUILD_DIR}
                        DEPENDS ${WEBP_LOCAL_TARGET}
                        COMMENT "COPY AND PATCH WEBP"
                        COMMAND cp ${WEBP_LOCAL_TARGET} ${WEBP_TARGET}
                        COMMAND patchelf --set-soname libwebp_lwe.so ${WEBP_TARGET}
    )

    ADD_CUSTOM_TARGET (libwebp_lwe
                       DEPENDS ${WEBP_TARGET}
                       COMMENT "WEBP TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${WEBP_DIR}/src)
ENDIF()


#######################################################
# LINK THIRD PARTY LIBRARIES
#######################################################
SET (STARFISH_LIBRARIES_THIRD_PARTY ${GC_TARGET} skia_matrix clipper escargot)
SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} mp4parse webm)

IF (${BUILD_CAIRO} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${CAIRO_TARGET} -lpixman-1)
ENDIF()

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${NANOMSG_TARGET})
ENDIF()

IF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND (${BACKEND} STREQUAL "uv_cairo_gl")))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ELSEIF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (CMAKE_SYSTEM_NAME STREQUAL "Tizen" AND (${BACKEND} STREQUAL "uv_cairo_gl")))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()

IF (${BACKEND} STREQUAL "glib_cairo_gl")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY})
ENDIF()

IF (${WEBRTC} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${WEBRTC_TARGET})
ENDIF()

IF (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR ${CUSTOM} STREQUAL "prod_tv")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${LIBWEBSOCKETS_TARGET})
ENDIF()

IF (CMAKE_SYSTEM_NAME STREQUAL "Tizen" AND ${BACKEND} STREQUAL "flutter")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()

IF (${WORKER} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()
