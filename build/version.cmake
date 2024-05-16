CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

#######################################################
# VERSION
FILE(READ ${CMAKE_SOURCE_DIR}/packaging/lightweight-web-engine.spec SPEC_FILE)
STRING(REGEX MATCH "Version: [ ]*([0-9]+\\.[0-9]+\\.[0-9]+)" MACHED_VERSION ${SPEC_FILE})
IF(MACHED_VERSION)
    SET(LWE_VERSION ${CMAKE_MATCH_1})
ELSE()
    MESSAGE(FATAL_ERROR "Not found version string in spec file.")
ENDIF()

MESSAGE(STATUS "LWE_VERSION: ${LWE_VERSION}")
#######################################################
