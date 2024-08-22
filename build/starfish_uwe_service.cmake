CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET (UWE_PACKAGE_DIR "${TIZEN_RW_APP_DIR}/${UWE_PACKAGE_ID}")
SET (UWE_MOUNT_PATH "${TIZEN_DATA_DIR}/lwe/update")
MESSAGE (STATUS "UWE_PACKAGE_DIR: ${UWE_PACKAGE_DIR}")
MESSAGE (STATUS "UWE_MOUNT_PATH: ${UWE_MOUNT_PATH}")
MESSAGE (STATUS "Create lightweight-web-engine-update.service...")

CONFIGURE_FILE (build/tizen/systemd/lightweight-web-engine-update.service.in ${OUTPUT_DIRECTORY}/lightweight-web-engine-update.service)

IF (${HOST} STREQUAL "tizen" AND ${CUSTOM} STREQUAL "prod_tv")
    FILE(READ build/tizen/systemd/path-hash-sign hash)
    FILE(READ ${OUTPUT_DIRECTORY}/lightweight-web-engine-update.service content)
    SET(content "${hash}\n${content}")
    FILE(WRITE ${OUTPUT_DIRECTORY}/lightweight-web-engine-update.service "${content}")
ENDIF()
