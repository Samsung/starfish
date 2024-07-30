CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET(UWE_PACKAGE_ID "org.tizen.lightweight-web-engine")
SET(TIZEN_VERSION "${TIZEN_MAJOR_VERSION}.${TIZEN_MINOR_VERSION}")
SET(UWE_TPK_ROOT ${OUTPUT_DIRECTORY}/uwe_tpk_root)
SET(UWE_TPK_VERSION ${LWE_VERSION})
SET(UWE_TPK_NAME ${UWE_PACKAGE_ID}-${UWE_TPK_VERSION}.${ARCH}.tpk)
SET(HASH_SIGNER_SH "/usr/bin/hash-signer.sh")
GET_TARGET_PROPERTY(STARFISH_API_OUTPUT_NAME starfish_api.shared_library OUTPUT_NAME)

CONFIGURE_FILE(build/tizen/tpk/uwe_tizen-manifest.xml.in ${OUTPUT_DIRECTORY}/uwe_tizen-manifest.xml)

ADD_CUSTOM_TARGET (uwe_tpk_root
    COMMAND echo "Make uwe tpk root..."
    COMMAND rm -rf ${UWE_TPK_ROOT}
    COMMAND install -d ${UWE_TPK_ROOT}
    COMMAND install -m 0644 ${OUTPUT_DIRECTORY}/uwe_tizen-manifest.xml ${UWE_TPK_ROOT}/tizen-manifest.xml
    COMMAND install -d ${UWE_TPK_ROOT}/bin
    COMMAND install -m 0755 ${OUTPUT_DIRECTORY}/VERSION ${UWE_TPK_ROOT}
    COMMAND install -d ${UWE_TPK_ROOT}/lib/
    COMMAND install -m 0644 ${OUTPUT_DIRECTORY}/lib/*.so* ${UWE_TPK_ROOT}/lib/
    COMMAND strip -v --strip-all ${UWE_TPK_ROOT}/lib/*.so
    COMMAND install -d ${UWE_TPK_ROOT}/res/
    COMMAND mksquashfs ${UWE_TPK_ROOT}/lib/ ${UWE_TPK_ROOT}/res/lwe_update.img -all-root
    COMMAND rm -rf ${UWE_TPK_ROOT}/lib/
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(signed_uwe_tpk_root
    COMMAND echo "Sign uwe_tpk_root using hash-signer..."
    COMMAND ${HASH_SIGNER_SH} -a -d -p platform ${UWE_TPK_ROOT}
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(uwe_tpk
    COMMAND echo "Packge uwe_tpk_root.."
    COMMAND rm -f ${OUTPUT_DIRECTORY}/*.tpk
    COMMAND pushd ${UWE_TPK_ROOT}
    COMMAND zip -yr ${UWE_TPK_NAME} *
    COMMAND mv ${UWE_TPK_NAME} ../
    COMMAND popd
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(resigned_uwe_tpk
    COMMAND echo "Resign tpk using tpkresigner..."
    COMMAND tpkresigner -a -d -p platform -n ${UWE_PACKAGE_ID} ${OUTPUT_DIRECTORY}/${UWE_TPK_NAME}
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET (starfish.uwe.tpk
    COMMENT "starfish.uwe.tpk TARGET"
)

IF (${CUSTOM} STREQUAL "prod_tv")
    ADD_DEPENDENCIES (starfish.uwe.tpk resigned_uwe_tpk)
    ADD_DEPENDENCIES (resigned_uwe_tpk uwe_tpk)
    ADD_DEPENDENCIES (uwe_tpk signed_uwe_tpk_root)
    ADD_DEPENDENCIES (signed_uwe_tpk_root uwe_tpk_root)
    ADD_DEPENDENCIES (uwe_tpk_root starfish.executable)
ELSE()
    ADD_DEPENDENCIES (starfish.uwe.tpk uwe_tpk)
    ADD_DEPENDENCIES (uwe_tpk signed_uwe_tpk_root)
    ADD_DEPENDENCIES (signed_uwe_tpk_root uwe_tpk_root)
    ADD_DEPENDENCIES (uwe_tpk_root starfish.executable)
ENDIF()
