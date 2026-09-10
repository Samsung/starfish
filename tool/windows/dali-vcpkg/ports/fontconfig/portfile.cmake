
include(vcpkg_common_functions)

set(FONTCONFIG_VERSION 2.13.92)
vcpkg_download_distfile(ARCHIVE
    URLS "https://www.freedesktop.org/software/fontconfig/release/fontconfig-${FONTCONFIG_VERSION}.tar.gz"
    FILENAME "fontconfig-${FONTCONFIG_VERSION}.tar.gz"
    SHA512 bc6719c04489c3baba9c59ee722a2fce2707f92f95432ab2c00d3f13353c603ccd1c43f751231a42c22fac7908561f7efe734718142061934e9809cac9b6071d
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    REF ${FONTCONFIG_VERSION}
    PATCHES
      fcobjtypehash.patch
      0002_[fontconfig]_SourceCompileFix.patch
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DFC_INCLUDE_DIR=${CMAKE_CURRENT_LIST_DIR}/include
    OPTIONS_DEBUG
        -DFC_SKIP_TOOLS=ON
        -DFC_SKIP_HEADERS=ON
)

vcpkg_install_cmake()

# Starfish embeds fonts.conf + conf.d from this standard vcpkg location.
file(COPY ${CURRENT_PACKAGES_DIR}/tools/fontconfig/fonts DESTINATION ${CURRENT_PACKAGES_DIR}/etc)

vcpkg_fixup_cmake_targets(CONFIG_PATH share/unofficial-fontconfig TARGET_PATH share/unofficial-fontconfig)

vcpkg_copy_pdbs()

if(VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
    foreach(HEADER fcfreetype.h fontconfig.h)
        file(READ ${CURRENT_PACKAGES_DIR}/include/fontconfig/${HEADER} FC_HEADER)
        if(NOT VCPKG_CMAKE_SYSTEM_NAME OR VCPKG_CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
            string(REPLACE "#define FcPublic" "#define FcPublic __declspec(dllimport)" FC_HEADER "${FC_HEADER}")
        else()
            string(REPLACE "#define FcPublic" "#define FcPublic __attribute__((visibility(\"default\")))" FC_HEADER "${FC_HEADER}")
        endif()
        file(WRITE ${CURRENT_PACKAGES_DIR}/include/fontconfig/${HEADER} "${FC_HEADER}")
    endforeach()
endif()

file(COPY ${SOURCE_PATH}/COPYING DESTINATION ${CURRENT_PACKAGES_DIR}/share/fontconfig)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/fontconfig/COPYING ${CURRENT_PACKAGES_DIR}/share/fontconfig/copyright)

vcpkg_test_cmake(PACKAGE_NAME unofficial-fontconfig)
