vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

# The gitlink pins the package; no expiring downloads or GNU tools are needed.
get_filename_component(STARFISH_ROOT "${CURRENT_PORT_DIR}/../../.." ABSOLUTE)
set(FFMPEG_ROOT "${STARFISH_ROOT}/third_party/windows/ffmpeg")
if(NOT EXISTS "${FFMPEG_ROOT}/manifest.json")
    message(FATAL_ERROR "Run git submodule update --init third_party/windows/ffmpeg before configuring Windows")
endif()
file(SHA256 "${FFMPEG_ROOT}/SHA256SUMS" FFMPEG_PACKAGE_HASH)
if(NOT FFMPEG_PACKAGE_HASH STREQUAL "b9bc2a00c3d937db53a20cbf90d58532cff4febed8698efb00ac984debabe2a2")
    message(FATAL_ERROR "FFmpeg gitlink changed; update the prebuilt port and its ABI pin together")
endif()
set(FFMPEG_ARCH_ROOT "${FFMPEG_ROOT}/${VCPKG_TARGET_ARCHITECTURE}")
if(NOT VCPKG_TARGET_ARCHITECTURE MATCHES "^(x86|x64)$")
    message(FATAL_ERROR "The FFmpeg prebuilt package supports x86 and x64 only")
endif()
file(INSTALL "${FFMPEG_ARCH_ROOT}/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(GLOB FFMPEG_DLLS "${FFMPEG_ARCH_ROOT}/bin/*.dll")
file(INSTALL ${FFMPEG_DLLS} DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
file(INSTALL ${FFMPEG_DLLS} DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib" "${CURRENT_PACKAGES_DIR}/debug/lib")
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    # MSVC /OPT:REF needs native import libraries instead of dlltool output.
    find_program(FFMPEG_LIB_EXE NAMES lib.exe REQUIRED)
    foreach(COMPONENT avcodec avformat avutil swscale swresample)
        file(GLOB FFMPEG_DEF "${FFMPEG_ARCH_ROOT}/lib/${COMPONENT}-*.def")
        vcpkg_execute_required_process(
            COMMAND "${FFMPEG_LIB_EXE}" /nologo /machine:x64
                "/def:${FFMPEG_DEF}" "/out:${CURRENT_PACKAGES_DIR}/lib/${COMPONENT}.lib"
            WORKING_DIRECTORY "${CURRENT_BUILDTREES_DIR}"
            LOGNAME "import-${COMPONENT}")
    endforeach()
else()
    file(GLOB FFMPEG_LIBS "${FFMPEG_ARCH_ROOT}/lib/*.lib")
    file(INSTALL ${FFMPEG_LIBS} DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
endif()
file(GLOB FFMPEG_LIBS "${CURRENT_PACKAGES_DIR}/lib/*.lib")
file(INSTALL ${FFMPEG_LIBS} DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
file(INSTALL "${CURRENT_PORT_DIR}/starfish-ffmpeg-prebuilt-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${FFMPEG_ROOT}/manifest.json" "${FFMPEG_ROOT}/SHA256SUMS"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
if(EXISTS "${FFMPEG_ARCH_ROOT}/LICENSE.txt")
    file(INSTALL "${FFMPEG_ARCH_ROOT}/LICENSE.txt"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
else()
    file(INSTALL "${FFMPEG_ROOT}/LICENSE.txt"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
endif()
