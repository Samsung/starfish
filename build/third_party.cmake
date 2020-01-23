CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

# ESCARGOT THIRDPARTY
IF (${HOST} STREQUAL "linux" AND ((${BACKEND} STREQUAL "glfw_cairo_gl") OR (${BACKEND} STREQUAL "efl_cairo_gl") OR (${BACKEND} STREQUAL "efl_headless") OR (${BACKEND} STREQUAL "efl_skia_gl") OR (${BACKEND} STREQUAL "efl_skia_gb")))
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
IF (NOT (${BACKEND} STREQUAL "efl_skia_gl" OR ${BACKEND} STREQUAL "efl_skia_gb"))
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
# Nanomsg is not used for tizen
IF (NOT ${HOST} STREQUAL "tizen")
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
    SET (NANOMSG_CUSTOM -DNN_ENABLE_DOC=OFF -DNN_TESTS=OFF -DNN_TOOLS=OFF -DNN_ENABLE_GETADDRINFO_A=OFF -DCMAKE_INSTALL_PREFIX=/dist)

    SET (NANOMSG_BUILDDIR ${THIRD_PARTY_ROOT}/nanomsg/out/${HOST}/${ARCH}/${MODE}.shared)
    SET (NANOMSG_LOCAL_TARGET ${NANOMSG_BUILDDIR}/libnanomsg.so)
    SET (NANOMSG_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libnanomsg.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${NANOMSG_LOCAL_TARGET}
                        COMMENT "BUILD NANOMSG"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${NANOMSG_BUILDDIR}
                        COMMAND cd ${NANOMSG_BUILDDIR} && cmake ../../../../ -DCMAKE_C_FLAGS=${NANOMSG_CFLAGS} -DCMAKE_CXX_FLAGS=${NANOMSG_CFLAGS} ${NANOMSG_CUSTOM}
                        COMMAND cd ${NANOMSG_BUILDDIR} && make -j
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${NANOMSG_TARGET}
                        DEPENDS ${NANOMSG_LOCAL_TARGET}
                        COMMENT "COPY NANOMSG"
                        COMMAND cp -P ${NANOMSG_LOCAL_TARGET}* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
                        COMMENT "INSTALL NANOMSG"
                        COMMAND cd ${NANOMSG_BUILDDIR} && make DESTDIR=${NANOMSG_BUILDDIR}/../../../../ install
    )

    ADD_CUSTOM_TARGET (nanomsg
                       DEPENDS ${NANOMSG_TARGET}
                       COMMAND echo "NANOMSG TARGET"
    )
ENDIF()

#######################################################
# LIBWEBSOCKETS
#######################################################
IF (${ARCH} STREQUAL "x64" OR ${CUSTOM} STREQUAL "prod_tv")
    SET(LIBWEBSOCKETS_DIR ${THIRD_PARTY_ROOT}/libwebsockets/)
    SET(LIBWEBSOCKETS_BUILD_PATH ${LIBWEBSOCKETS_DIR}/build)
    SET(LIBWEBSOCKETS_LOCAL_TARGET ${LIBWEBSOCKETS_BUILD_PATH}/lib/libwebsockets.so)
    SET(LIBWEBSOCKETS_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebsockets.so)
IF (${WEBRTC} STREQUAL "1")
    SET(OPENSSL_LIB_CUSTOM "-DLWS_OPENSSL_LIBRARIES=\"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.so\"")
    SET(LIBWEBSOCKETS_BUILD_OPTION -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF -DOPENSSL_ROOT_DIR=${THIRD_PARTY_ROOT}/openssl/out/${HOST}/${ARCH}/${MODE} -DLWS_OPENSSL_INCLUDE_DIRS=${THIRD_PARTY_ROOT}/openssl/out/${HOST}/${ARCH}/${MODE}/include)
    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
                        DEPENDS openssl
                        WORKING_DIRECTORY ${LIBWEBSOCKETS_DIR}
                        COMMENT "BUILD LIBWEBSOCKETS"
                        COMMAND echo "BUILD LIBWEBSOCKETS"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_PATH}
                        COMMAND cd ${LIBWEBSOCKETS_BUILD_PATH}
                        COMMAND ${CMAKE_COMMAND} -G Ninja .. ${LIBWEBSOCKETS_BUILD_OPTION} "${OPENSSL_LIB_CUSTOM}"
                        COMMAND ninja
    )
ELSE()
    SET(LIBWEBSOCKETS_BUILD_OPTION -DLWS_MAX_SMP=1 -DLWS_CLIENT_HTTP_PROXYING:BOOL=OFF)
    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_LOCAL_TARGET}
                        WORKING_DIRECTORY ${LIBWEBSOCKETS_DIR}
                        COMMENT "BUILD LIBWEBSOCKETS"
                        COMMAND echo "BUILD LIBWEBSOCKETS"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBWEBSOCKETS_BUILD_PATH}
                        COMMAND cd ${LIBWEBSOCKETS_BUILD_PATH}
                        COMMAND ${CMAKE_COMMAND} -G Ninja .. ${LIBWEBSOCKETS_BUILD_OPTION}
                        COMMAND ninja
    )
ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${LIBWEBSOCKETS_TARGET}
                        DEPENDS ${LIBWEBSOCKETS_LOCAL_TARGET}
                        COMMENT "COPY LIBWEBSOCKETS"
                        COMMAND cp -P ${LIBWEBSOCKETS_BUILD_PATH}/lib/* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (libwebsockets
                        DEPENDS ${LIBWEBSOCKETS_TARGET}
                        COMMAND echo "LIBWEBSOCKETS TARGET"
    )
ENDIF()

#######################################################
# LIBTUV
#######################################################
IF (${ARCH} STREQUAL "x64" AND ((${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "glfw_cairo_gl")
        OR (LWE_DEFINES_MODE MATCHES STARFISH_ENABLE_SERVICE_WORKER)))
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
    SET (TUV_LOCAL_TARGET ${TUV_DIR}/build/noarch-tizen/${MODE}/lib/libtuv.so)
    SET (TUV_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libtuv.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_LOCAL_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        COMMENT "BUILD TUV"
                        COMMAND make clean
                        COMMAND cp ${TUV_DIR}/config/tizen/packaging/libtuv.pc.in .
                        COMMAND make -j TUV_BUILD_TYPE=${MODE} TUV_BUILDTESTER=no TUV_CREATE_SHARED_LIB=yes TUV_BOARD=None TUV_PLATFORM=noarch-tizen
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${TUV_TARGET}
                        WORKING_DIRECTORY ${TUV_DIR}
                        DEPENDS ${TUV_LOCAL_TARGET}
                        COMMENT "COPY TUV"
                        COMMAND cp ${TUV_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (tuv
                       DEPENDS ${TUV_TARGET}
                       COMMAND echo "TUV TARGET"
    )
ENDIF()


#######################################################
# LIBSKIA
#######################################################
IF (${HOST} STREQUAL "linux" AND (${BACKEND} STREQUAL "efl_skia_gl" OR ${BACKEND} STREQUAL "efl_skia_gb"))
    SET (SKIA_DIR ${THIRD_PARTY_ROOT}/android/skia/)
    SET (SKIA_BUILD_ARGS "is_component_build=true" "target_cpu=\\\"x64\\\"")
    SET (SKIA_BUILD_TYPE "Release")
    IF (${MODE} STREQUAL "debug")
        SET (SKIA_BUILD_TYPE "Debug")
        SET (SKIA_BUILD_ARGS ${SKIA_BUILD_ARGS} "is_debug=true")
    ELSE()
        SET (SKIA_BUILD_TYPE "Release")
        SET (SKIA_BUILD_ARGS ${SKIA_BUILD_ARGS} "is_debug=false")
    ENDIF()

    IF (${BACKEND} STREQUAL "efl_skia_gl")
        SET (SKIA_BUILD_ARGS ${SKIA_BUILD_ARGS} "is_rgba=true")
    ELSEIF(${BACKEND} STREQUAL "efl_skia_gb")
        SET (SKIA_BUILD_ARGS ${SKIA_BUILD_ARGS} "is_rgba=false")
    ENDIF()

    SET(SKIA_LOCAL_TARGET ${SKIA_DIR}/out/${SKIA_BUILD_TYPE}/Shared/libskia.so)
    SET(SKIA_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libskia.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${SKIA_LOCAL_TARGET}
                        WORKING_DIRECTORY ${SKIA_DIR}
                        COMMENT "BUILD SKIA"
                        COMMAND echo "BUILD SKIA"
                        COMMAND bin/gn gen out/${SKIA_BUILD_TYPE}/Shared --args="${SKIA_BUILD_ARGS}"
                        COMMAND ninja -d explain -C out/${SKIA_BUILD_TYPE}/Shared -t clean
                        COMMAND ninja -d explain -C out/${SKIA_BUILD_TYPE}/Shared
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${SKIA_TARGET}
                        WORKING_DIRECTORY ${SKIA_DIR}
                        DEPENDS ${SKIA_LOCAL_TARGET}
                        COMMENT "COPY SKIA"
                        COMMAND cp ${SKIA_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (skia
                       DEPENDS ${SKIA_TARGET}
                       COMMAND echo "SKIA TARGET"
    )
ENDIF()

#######################################################
# ESCARGOT
#######################################################
SET (ESCARGOT_MODE ${MODE})
SET (ESCARGOT_ARCH ${ARCH})
SET (ESCARGOT_OUTPUT static_lib)
IF (${HOST} STREQUAL "linux")
    SET (ESCARGOT_HOST ${HOST})
ELSE()
    SET (ESCARGOT_HOST tizen_obs)
ENDIF()
ADD_SUBDIRECTORY (third_party/escargot)

#######################################################
# OpenSSL 1.1
#######################################################
# Used when a target platform does not have openssl 1.1.
# Currently, Ubuntu 16.04 and prod_tv do not have openssl 1.1.
IF (${WEBRTC} STREQUAL "1")
    SET (OPENSSL_DIR ${THIRD_PARTY_ROOT}/openssl)
    SET (OPENSSL_BUILD_PATH out/${HOST}/${ARCH}/${MODE})
    SET (OPENSSL_LOCAL_TARGET ${OPENSSL_DIR}/${OPENSSL_BUILD_PATH}/libssl.so)
    SET (OPENSSL_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so)

    ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_LOCAL_TARGET}
                        WORKING_DIRECTORY ${OPENSSL_DIR}
                        COMMENT "BUILDING OPENSSL"
                        COMMAND echo "BUILDING OPENSSL"
                        COMMAND ${CMAKE_COMMAND} -E make_directory ${OPENSSL_BUILD_PATH}
                        COMMAND cd ${OPENSSL_BUILD_PATH}
                        COMMAND ../../../../config
                        COMMAND make -j
                        COMMAND cp -r ../../../../include .
    )

    ADD_CUSTOM_COMMAND (OUTPUT ${OPENSSL_TARGET}
                        WORKING_DIRECTORY ${OPENSSL_DIR}
                        DEPENDS ${OPENSSL_LOCAL_TARGET}
                        COMMENT "COPYING OPENSSL"
                        COMMAND cp -P ${OPENSSL_DIR}/${OPENSSL_BUILD_PATH}/lib*so* ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (openssl
                    DEPENDS ${OPENSSL_TARGET}
                    COMMAND echo "OPENSSL TARGET"
    )
ENDIF()

#######################################################
# WEBRTC
#######################################################

IF (${WEBRTC} STREQUAL "1")
    SET (WEBRTC_DIR ${THIRD_PARTY_ROOT}/webrtc/src/)

    IF (${HOST} STREQUAL "linux")
        SET (WEBRTC_BUILD_ARGS
            "target_cpu=\\\"x64\\\""
            "rtc_ssl_root=\\\"${OPENSSL_DIR}/${OPENSSL_BUILD_PATH}/include\\\""
        )
    ELSEIF (${HOST} STREQUAL "tizen")
        SET (WEBRTC_BUILD_ARGS
            "target_cpu=\\\"arm\\\""
            "rtc_ssl_root=\\\"${OPENSSL_DIR}/${OPENSSL_BUILD_PATH}/include\\\""
            # TODO: Use the platform include path for Tizen 5.5
        )
    ENDIF()

    IF (${MODE} STREQUAL "debug")
        SET (WEBRTC_BUILD_ARGS ${WEBRTC_BUILD_ARGS} "is_debug=true")
    ELSE()
        SET (WEBRTC_BUILD_ARGS ${WEBRTC_BUILD_ARGS} "is_debug=false")
    ENDIF()

    SET (WEBRTC_BUILD_ARGS ${WEBRTC_BUILD_ARGS}
        "is_clang=false"
        "treat_warnings_as_errors=false"
        "use_custom_libcxx=false"
        "use_udev=false"
        "use_ozone=true"
        "use_cxx11=true"
        "enable_iterator_debugging=true"
        "enable_nacl=false"
        "use_glib=true"
        "use_rtti=false"
        "use_sysroot=false"
        "build_with_chromium=false"
        "rtc_build_ssl=false"
        "rtc_enable_protobuf=false"
        "rtc_build_json=true"
        "use_system_libjpeg=true"
        "use_system_freetype=true"
        "use_system_harfbuzz=true"
        "rtc_include_tests=false"
    )

    SET(WEBRTC_BUILD_PATH out/${HOST}/${ARCH}/${MODE})
    SET(WEBRTC_LOCAL_TARGET ${WEBRTC_DIR}/${WEBRTC_BUILD_PATH}/libwebrtc.so)
    SET(WEBRTC_TARGET ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwebrtc.so)

    IF (${HOST} STREQUAL "linux")
        ADD_CUSTOM_COMMAND (OUTPUT ${WEBRTC_LOCAL_TARGET}
                            WORKING_DIRECTORY ${WEBRTC_DIR}
                            DEPENDS ${OPENSSL_TARGET}
                            COMMENT "BUILD WEBRTC"
                            COMMAND echo "BUILD WEBRTC"
                            COMMAND third_party/depot_tools/gn gen ${WEBRTC_BUILD_PATH} --no-parallel --args="${WEBRTC_BUILD_ARGS}"
                            COMMAND third_party/depot_tools/ninja -C ${WEBRTC_BUILD_PATH} webrtc
                            COMMAND ${COMPILER} -shared -fPIC -o ${WEBRTC_BUILD_PATH}/libwebrtc.so -Wl,-soname,libwebrtc.so -Wl,--whole-archive ${WEBRTC_BUILD_PATH}/obj/libwebrtc.a  -Wl,--no-whole-archive ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.so -lpthread -lm -ljpeg
        )
    ELSEIF (${HOST} STREQUAL "tizen")
        ADD_CUSTOM_COMMAND (OUTPUT ${WEBRTC_LOCAL_TARGET}
                            WORKING_DIRECTORY ${WEBRTC_DIR}
                            DEPENDS ${OPENSSL_TARGET}
                            COMMENT "BUILD WEBRTC"
                            COMMAND echo "BUILD WEBRTC"
                            COMMAND buildtools/armv7l/gn gen ${WEBRTC_BUILD_PATH} --no-parallel --args="${WEBRTC_BUILD_ARGS}"
                            COMMAND ninja -C ${WEBRTC_BUILD_PATH} webrtc
                            # NOTE: for Tizen 5.5, use: -lssl -lcrypto
                            COMMAND ${COMPILER} -shared -fPIC -o ${WEBRTC_BUILD_PATH}/libwebrtc.so -Wl,-soname,libwebrtc.so -Wl,--whole-archive ${WEBRTC_BUILD_PATH}/obj/libwebrtc.a  -Wl,--no-whole-archive ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libssl.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcrypto.so -lpthread -lm -ljpeg
        )
    ENDIF()

    ADD_CUSTOM_COMMAND (OUTPUT ${WEBRTC_TARGET}
                        WORKING_DIRECTORY ${WEBRTC_DIR}
                        DEPENDS ${WEBRTC_LOCAL_TARGET}
                        COMMENT "COPY WEBRTC"
                        COMMAND cp ${WEBRTC_LOCAL_TARGET} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/.
    )

    ADD_CUSTOM_TARGET (webrtc
                       DEPENDS ${WEBRTC_TARGET}
                       COMMAND echo "WEBRTC TARGET"
    )
ENDIF()

#######################################################
# LINK THIRD PARTY LIBRARIES
#######################################################
SET (STARFISH_LIBRARIES_THIRD_PARTY ${GC_TARGET} clipper escargot)
SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} mp4parse webm)

IF (NOT (${BACKEND} STREQUAL "efl_skia_gl" OR ${BACKEND} STREQUAL "efl_skia_gb"))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} skia_matrix)
ENDIF()

IF (${BACKEND} STREQUAL "efl_skia_gl" OR ${BACKEND} STREQUAL "efl_skia_gb")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${SKIA_TARGET})
ENDIF()

IF (NOT ${HOST} STREQUAL "tizen")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${NANOMSG_TARGET})
ENDIF()

IF (${ARCH} STREQUAL "x64" AND (${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "glfw_cairo_gl"))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ELSEIF (${HOST} STREQUAL "tizen" AND (${BACKEND} STREQUAL "ecore_wayland2_cairo_gl" OR ${BACKEND} STREQUAL "dali"))
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${TUV_TARGET})
ENDIF()

IF (${BACKEND} MATCHES "efl_cairo" OR ${BACKEND} STREQUAL "dali" OR ${BACKEND} STREQUAL "glfw_cairo_gl" OR ${BACKEND} STREQUAL "ecore_wayland2_cairo_gl")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY})
ENDIF()

IF (${WEBRTC} STREQUAL "1")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${WEBRTC_TARGET})
ENDIF()

IF (${ARCH} STREQUAL "x64")
    SET (STARFISH_LIBRARIES_THIRD_PARTY ${STARFISH_LIBRARIES_THIRD_PARTY} ${LIBWEBSOCKETS_TARGET})
ENDIF()
