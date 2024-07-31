CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET (UWE_PACKAGE_DIR "${TIZEN_RW_APP_DIR}/${UWE_PACKAGE_ID}")
SET (UWE_MOUNT_PATH "${TIZEN_DATA_DIR}/lwe/update")
MESSAGE (STATUS "UWE_PACKAGE_DIR: ${UWE_PACKAGE_DIR}")
MESSAGE (STATUS "UWE_MOUNT_PATH: ${UWE_MOUNT_PATH}")
MESSAGE (STATUS "Create lightweight-web-engine-update.service...")

CONFIGURE_FILE (build/tizen/systemd/lightweight-web-engine-update.service.in ${OUTPUT_DIRECTORY}/lightweight-web-engine-update.service)
