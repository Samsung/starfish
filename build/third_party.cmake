CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

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
FILE (GLOB MP4PARSE_LIST ${THIRD_PARTY_ROOT}/MP4Parse/source/MP4*.cpp)
ADD_LIBRARY (mp4parse SHARED ${MP4PARSE_LIST})
TARGET_INCLUDE_DIRECTORIES (mp4parse PUBLIC ${THIRD_PARTY_ROOT}/MP4Parse/source/include)
TARGET_COMPILE_DEFINITIONS (mp4parse PUBLIC ${THIRD_PARTY_DEFINITIONS})
TARGET_COMPILE_OPTIONS (mp4parse PUBLIC ${THIRD_PARTY_CXXFLAGS})


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


#######################################################
# NANOMSG
#######################################################
# Nanomsg is used for SharedWorker, ServiceWorker and Inspector
IF (${ARCH} STREQUAL "x64" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")
    SET (NANOMSG_BUILDDIR ${OUTPUT_DIRECTORY}/nanomsg/out/${HOST}/${ARCH}/${MODE}.shared)
    SET (NANOMSG_LOCAL_TARGET ${NANOMSG_BUILDDIR}/libnanomsg.so)
    SET (NANOMSG_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libnanomsg.so)

    SET (NANOMSG_CFLAGS_COMMON "-g3 -fPIC")
    IF (${CUSTOM} STREQUAL "unified_wearable")
        SET (NANOMSG_CFLAGS_CUSTOM "-Os")
    ENDIF()

    IF (${ARCH} STREQUAL "x86")
        SET (NANOMSG_CFLAGS_ARCH "-m32")
    ELSEIF (${ARCH} STREQUAL "arm")
        SET (NANOMSG_CFLAGS_ARCH "-march=armv7-a -mthumb -finline-limit=64")
    ENDIF()

    IF (${MODE} STREQUAL "debug")
        SET (NANOMSG_CFLAGS_MODE "-O0")
    ELSE()
        SET (NANOMSG_CFLAGS_MODE "-O2")
    ENDIF()

    SET (NANOMSG_CFLAGS "${NANOMSG_CFLAGS_COMMON} ${NANOMSG_CFLAGS_CUSTOM} ${NANOMSG_CFLAGS_ARCH} ${NANOMSG_CFLAGS_MODE}")
    SET (NANOMSG_CUSTOM -DNN_ENABLE_DOC=OFF -DNN_TESTS=OFF -DNN_TOOLS=OFF -DNN_ENABLE_GETADDRINFO_A=OFF -DCMAKE_INSTALL_PREFIX=${NANOMSG_BUILDDIR}/dist)

    ADD_CUSTOM_COMMAND (OUTPUT ${NANOMSG_LOCAL_TARGET}
                        COMMENT "BUILD NANOMSG"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${NANOMSG_BUILDDIR}
                        COMMAND cd ${THIRD_PARTY_ROOT}/nanomsg/ && cmake ${CMAKE_COMMAND} -S . -B${NANOMSG_BUILDDIR} -DCMAKE_C_FLAGS=${NANOMSG_CFLAGS} -DCMAKE_CXX_FLAGS=${NANOMSG_CFLAGS} ${NANOMSG_CUSTOM}
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

IF (${ARCH} STREQUAL "x64" OR ${CUSTOM} STREQUAL "prod_tv" OR ${CUSTOM} STREQUAL "unified_tv" OR ${CUSTOM} STREQUAL "unified_mobile")
    SET(LIBWEBSOCKETS_SOURCE_DIR ${THIRD_PARTY_ROOT}/libwebsockets/)
    SET(LIBWEBSOCKETS_BUILD_DIR ${OUTPUT_DIRECTORY}/libwebsockets/)
    SET(LIBWEBSOCKETS_BUILD_OUTDIR ${OUTPUT_DIRECTORY}/libwebsockets/build/${HOST}/${ARCH}/${MODE})
    SET(LIBWEBSOCKETS_LOCAL_TARGET ${LIBWEBSOCKETS_BUILD_OUTDIR}/lib/libwebsockets.so)
    SET(LIBWEBSOCKETS_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebsockets_lwe.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
                        COMMENT "COPY LIBWEBSOCKETS SOURCE"
                        COMMAND cp -r ${LIBWEBSOCKETS_SOURCE_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND sed -i "s/hidden/default/" ${LIBWEBSOCKETS_BUILD_DIR}/include/libwebsockets.h
                        COMMAND touch ${LIBWEBSOCKETS_BUILD_DIR}/copied
    )

    IF (${ARCH} STREQUAL "x64")
        SET (OPENSSL_LIB_CUSTOM "-DLWS_OPENSSL_LIBRARIES=\"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.so\"")
        SET (OPENSSL_BUILD_PATH ${OUTPUT_DIRECTORY}/openssl/out/${HOST}/${ARCH}/${MODE})
        SET (LIBWEBSOCKETS_BUILD_OPTION -DSTARFISH_CUSTOM=1 -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF -DLWS_HAVE_VISIBILITY:BOOL=ON -DLWS_STATIC_PIC:BOOL=OFF -DOPENSSL_ROOT_DIR=${OPENSSL_BUILD_PATH} -DLWS_OPENSSL_INCLUDE_DIRS=${OPENSSL_BUILD_PATH}/include)
        ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
                            DEPENDS openssl ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
                            WORKING_DIRECTORY ${LIBWEBSOCKETS_BUILD_DIR}
                            COMMENT "BUILD LIBWEBSOCKETS"
                            COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_OUTDIR}
                            COMMAND ${CMAKE_COMMAND} -S . -B${LIBWEBSOCKETS_BUILD_OUTDIR} -G Ninja ${LIBWEBSOCKETS_BUILD_OPTION} "${OPENSSL_LIB_CUSTOM}"
                            COMMAND ${CMAKE_COMMAND} --build ${LIBWEBSOCKETS_BUILD_OUTDIR}
        )
    ELSE()
        SET (LIBWEBSOCKETS_BUILD_OPTION -DSTARFISH_CUSTOM=1 -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF -DLWS_HAVE_VISIBILITY:BOOL=ON -DLWS_STATIC_PIC:BOOL=OFF)
        ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
                            DEPENDS ${LIBWEBSOCKETS_BUILD_DIR}/libwebsocket_copied
                            WORKING_DIRECTORY ${LIBWEBSOCKETS_BUILD_DIR}
                            COMMENT "BUILD LIBWEBSOCKETS"
                            COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_OUTDIR}
                            COMMAND ${CMAKE_COMMAND} -S . -B${LIBWEBSOCKETS_BUILD_OUTDIR} -G Ninja ${LIBWEBSOCKETS_BUILD_OPTION}
                            COMMAND ${CMAKE_COMMAND} --build ${LIBWEBSOCKETS_BUILD_OUTDIR}
        )
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_TARGET}
                        DEPENDS ${LIBWEBSOCKETS_LOCAL_TARGET}
                        COMMENT "COPY LIBWEBSOCKETS"
                        COMMAND cp ${LIBWEBSOCKETS_LOCAL_TARGET} ${LIBWEBSOCKETS_TARGET}
    )

    ADD_CUSTOM_TARGET (libwebsockets
                        DEPENDS ${LIBWEBSOCKETS_TARGET}
                        COMMENT "LIBWEBSOCKETS TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${LIBWEBSOCKETS_BUILD_OUTDIR}/include)
ENDIF()

#######################################################
# LIBTUV
#######################################################
IF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (${ARCH} STREQUAL "x64" AND ((${BACKEND} STREQUAL "dali" OR (${BACKEND} STREQUAL "glfw_cairo_gl" OR ${BACKEND} STREQUAL "x11_cairo_gl"))
        OR ${WORKER} STREQUAL "1" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")))
    SET (TUV_DIR ${THIRD_PARTY_ROOT}/libtuv)
    SET (TUV_BUILD_DIR ${OUTPUT_DIRECTORY}/libtuv)
    SET (TUV_LOCAL_TARGET ${TUV_BUILD_DIR}/build/x86_64-linux/${MODE}/lib/libtuv.so)
    SET (TUV_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_LOCAL_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        # we should copy tuv repo because tuv make include file inside of tuv repo.
                        COMMAND cp -r ${TUV_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${TUV_BUILD_DIR} && make -j TUV_BUILD_TYPE=${MODE} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=x86_64-linux
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
ELSEIF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (${HOST} STREQUAL "tizen" AND (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "flutter"
        OR ${WORKER} STREQUAL "1" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")))
    SET (TUV_DIR ${THIRD_PARTY_ROOT}/libtuv)
    SET (TUV_BUILD_DIR ${OUTPUT_DIRECTORY}/libtuv)
    SET (TUV_LOCAL_TARGET ${TUV_BUILD_DIR}/build/noarch-tizen/${MODE}/lib/libtuv.so)
    SET (TUV_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_LOCAL_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        # we should copy tuv repo because tuv make include file inside of tuv repo.
                        COMMAND cp -r ${TUV_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cp ${TUV_DIR}/config/tizen/packaging/libtuv.pc.in ${TUV_BUILD_DIR}
                        COMMAND cd ${TUV_BUILD_DIR} && make -j TUV_BUILD_TYPE=${MODE} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=noarch-tizen
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
SET (ESCARGOT_MODE ${MODE})
SET (ESCARGOT_ARCH ${ARCH})
SET (ESCARGOT_OUTPUT static_lib)
IF (${ENABLE_WASM} STREQUAL "1")
    SET (ESCARGOT_WASM ON)
ENDIF()
IF (${ENABLE_CODECACHE} STREQUAL "1")
    SET (ESCARGOT_CODE_CACHE ON)
ENDIF()

IF (${HOST} STREQUAL "linux")
    SET (ESCARGOT_HOST ${HOST})
ELSE()
    SET (ESCARGOT_HOST tizen_obs)
ENDIF()

IF (${ENABLE_DEBUGGER} STREQUAL "1")
    SET (ESCARGOT_DEBUGGER ON)
ENDIF()

SET (ESCARGOT_USE_CUSTOM_LOGGING ON)

IF (${WORKER} STREQUAL "1")
    SET (ESCARGOT_THREADING ON)
    add_compile_options("-DGC_THREAD_ISOLATE=1")
ENDIF()

ADD_SUBDIRECTORY (third_party/escargot)

#######################################################
# OpenSSL 1.1
#######################################################
# Used when a target platform does not have openssl 1.1.
# Currently, Ubuntu 16.04 and prod_tv do not have openssl 1.1.
IF (${HOST} STREQUAL "linux")
    SET (OPENSSL_DIR ${THIRD_PARTY_ROOT}/openssl)
    SET (OPENSSL_BUILD_PATH ${OUTPUT_DIRECTORY}/openssl/out/${HOST}/${ARCH}/${MODE})
    SET (OPENSSL_LOCAL_TARGET ${OPENSSL_BUILD_PATH}/libssl.so)
    SET (OPENSSL_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_LOCAL_TARGET}
                        WORKING_DIRECTORY ${OPENSSL_DIR}
                        COMMENT "BUILDING OPENSSL"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${OPENSSL_BUILD_PATH}
                        COMMAND cd ${OPENSSL_BUILD_PATH}
                        COMMAND ${OPENSSL_DIR}/config
                        COMMAND make -j8
                        COMMAND cp -r ${OPENSSL_DIR}/include .
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_TARGET}
                        WORKING_DIRECTORY ${OPENSSL_DIR}
                        DEPENDS ${OPENSSL_LOCAL_TARGET}
                        COMMENT "COPYING OPENSSL"
                        COMMAND cp -P ${OPENSSL_BUILD_PATH}/lib*so* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (openssl
                    DEPENDS ${OPENSSL_TARGET}
                    COMMENT "OPENSSL TARGET"
    )

    SET (STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS ${STARFISH_THIRD_PARTY_LIBS_INCLUDE_DIRS} ${OPENSSL_BUILD_PATH}/include)
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
    SET(WEBRTC_BUILD_PATH libwebrtc/libs/${HOST}/${ARCH}/${MODE})
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

    IF(${ARCH} STREQUAL "arm")
        SET (PNG_OPTION ${PNG_OPTION}" -D_ARCH_ARM_ -mfpu=neon -DPNG_ARM_NEON=check")
    ELSEIF(${ARCH} STREQUAL "aarch64")
        SET (PNG_OPTION ${PNG_OPTION}" -D_ARCH_ARM_ -mfpu=neon -DPNG_ARM_NEON=on")
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${PNG_LOCAL_TARGET}
                        WORKING_DIRECTORY ${PNG_DIR}
                        COMMENT "BUILD PNG"
                        COMMAND cp -r ${PNG_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${PNG_BUILD_DIR} && CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ${CMAKE_COMMAND} ${PNG_OPTION}
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

    IF(${HOST} STREQUAL "tizen" AND ${CUSTOM} STREQUAL "prod_tv")
        SET (JPEG_OPTION ${JPEG_OPTION}" -DENABLE_COLOR_PICKER=TRUE -DCMAKE_C_FLAGS='-D_TIZEN_PRODUCT_TV -D_USE_PRODUCT_TV'")
    ENDIF()
    
    ADD_CUSTOM_COMMAND (OUTPUT ${JPEG_LOCAL_TARGET}
                        WORKING_DIRECTORY ${JPEG_DIR}
                        COMMENT "BUILD PNG"
                        COMMAND rm -rf ${JPEG_BUILD_DIR}
                        COMMAND cp -r ${JPEG_DIR} ${OUTPUT_DIRECTORY}
                        COMMAND cd ${JPEG_BUILD_DIR} && CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ${CMAKE_COMMAND} ${JPEG_OPTION}
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
# LINK THIRD PARTY LIBRARIES
#######################################################
SET (STARFISH_LIBRARIES_THIRD_PARTY ${GC_TARGET} skia_matrix clipper escargot)
SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} mp4parse webm)

IF (${BUILD_CAIRO} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${CAIRO_TARGET} -lpixman-1)
ENDIF()

IF (${ARCH} STREQUAL "x64" OR ${SHARED_WORKER} STREQUAL "1" OR ${SERVICE_WORKER} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${NANOMSG_TARGET})
ENDIF()

IF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (${ARCH} STREQUAL "x64" AND (${BACKEND} STREQUAL "dali" OR (${BACKEND} STREQUAL "glfw_cairo_gl" OR ${BACKEND} STREQUAL "x11_cairo_gl"))))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ELSEIF (${ENABLE_MULTI_BACKEND} STREQUAL "1" OR (${HOST} STREQUAL "tizen" AND (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali")))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()

IF (${BACKEND} MATCHES "efl_cairo" OR ${BACKEND} STREQUAL "dali" OR (${BACKEND} STREQUAL "glfw_cairo_gl" OR ${BACKEND} STREQUAL "x11_cairo_gl") OR ${BACKEND} STREQUAL "ecore_wayland2_cairo_gl")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY})
ENDIF()

IF (${WEBRTC} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${WEBRTC_TARGET})
ENDIF()

IF (${ARCH} STREQUAL "x64" OR ${CUSTOM} STREQUAL "prod_tv")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${LIBWEBSOCKETS_TARGET})
ENDIF()

IF (${HOST} STREQUAL "tizen" AND ${BACKEND} STREQUAL "flutter")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()

IF (${WORKER} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()
