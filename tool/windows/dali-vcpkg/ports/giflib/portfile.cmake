include(vcpkg_common_functions)

vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

set(GIFLIB_VERSION 5.1.4)
vcpkg_download_distfile(ARCHIVE
    URLS "https://sourceforge.net/projects/giflib/files/giflib-5.x/giflib-${GIFLIB_VERSION}.tar.gz/download"
    FILENAME "giflib-${GIFLIB_VERSION}.tar.gz"
    SHA512 d9a98a593bcfbbfb2fcbeef1382ca9669a6b85276512e6828c92869d16c7cfe980bbb318766fc176be3c9f7deff7878d2be8aeda1d40af2cc4ac723fe6121b83
)

if (VCPKG_CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" OR NOT VCPKG_CMAKE_SYSTEM_NAME)
    set(ADDITIONAL_PATCH "fix-compile-error.patch")
endif()

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    REF ${GIFLIB_VERSION}
    PATCHES
        msvc-guard-unistd-h.patch
        ${ADDITIONAL_PATCH}
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS_DEBUG
        -DGIFLIB_SKIP_HEADERS=ON
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(INSTALL ${SOURCE_PATH}/COPYING DESTINATION ${CURRENT_PACKAGES_DIR}/share/giflib RENAME copyright)
