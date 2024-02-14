CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET(PACKAGE_ID "org.tizen.lightweight-web-engine-shell")
SET(TIZEN_VERSION "${TIZEN_MAJOR_VERSION}.${TIZEN_MINOR_VERSION}")
SET(TPK_ROOT ${OUTPUT_DIRECTORY}/tpk_root)
SET(TPK_VERSOIN 0.0.1)
SET(TPK_NAME ${PACKAGE_ID}-${TPK_VERSOIN}.${ARCH}.tpk)
SET(HASH_SIGNER_SH "/usr/bin/hash-signer.sh")
GET_TARGET_PROPERTY(STARFISH_API_OUTPUT_NAME starfish_api.shared_library OUTPUT_NAME)

CONFIGURE_FILE(build/tizen/tpk/tizen-manifest.xml.in ${OUTPUT_DIRECTORY}/tizen-manifest.xml)

ADD_CUSTOM_TARGET (tpk_root
    COMMAND echo "Make tpk root..."
    COMMAND rm -rf ${TPK_ROOT}
    COMMAND install -d ${TPK_ROOT}
    COMMAND install -m 0644 ${OUTPUT_DIRECTORY}/tizen-manifest.xml ${TPK_ROOT}/tizen-manifest.xml
    COMMAND install -d ${TPK_ROOT}/bin
    COMMAND install -m 0755 ${OUTPUT_DIRECTORY}/${TARGETNAME} ${TPK_ROOT}/bin/
    COMMAND install -d ${TPK_ROOT}/lib/
    COMMAND install -m 0644 ${OUTPUT_DIRECTORY}/lib/*.so* ${TPK_ROOT}/lib/
    COMMAND strip -v --strip-all ${TPK_ROOT}/lib/*.so
    COMMAND mv ${TPK_ROOT}/lib/lib${TARGETNAME}.so ${TPK_ROOT}/lib/liblightweight-web-engine.so.1
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(kuep_signed_files
    COMMAND echo "Sign files using kuep_signer..."
    COMMAND kuep_signer.sh -tizen_major_ver ${TIZEN_MAJOR_VERSION} ${TPK_ROOT}/bin/${TARGETNAME}
    COMMAND find ${TPK_ROOT}/lib/ -name *.so* -exec kuep_signer.sh -tizen_major_ver ${TIZEN_MAJOR_VERSION} {} "\\;"
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(signed_tpk_root
    COMMAND echo "Sign tpk_root using hash-signer..."
    COMMAND ${HASH_SIGNER_SH} -a -d -p platform ${TPK_ROOT}
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(tpk
    COMMAND echo "Packge tpk_root.."
    COMMAND rm -f ${OUTPUT_DIRECTORY}/*.tpk
    COMMAND pushd ${TPK_ROOT}
    COMMAND zip -yr ${TPK_NAME} *
    COMMAND mv ${TPK_NAME} ../
    COMMAND popd
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(resigned_tpk
    COMMAND echo "Resign tpk using tpkresigner..."
    COMMAND tpkresigner -a -d -p platform -n ${PACKAGE_ID} ${OUTPUT_DIRECTORY}/${TPK_NAME}
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET (starfish.executable.tpk
    COMMENT "starfish.executable.tpk TARGET"
)

IF (${CUSTOM} STREQUAL "prod_tv")
    ADD_DEPENDENCIES (starfish.executable.tpk resigned_tpk)
    ADD_DEPENDENCIES (resigned_tpk tpk)
    ADD_DEPENDENCIES (tpk signed_tpk_root)
    ADD_DEPENDENCIES (signed_tpk_root kuep_signed_files)
    ADD_DEPENDENCIES (kuep_signed_files tpk_root)
    ADD_DEPENDENCIES (tpk_root starfish.executable)
ELSE()
    ADD_DEPENDENCIES (starfish.executable.tpk tpk)
    ADD_DEPENDENCIES (tpk signed_tpk_root)
    ADD_DEPENDENCIES (signed_tpk_root tpk_root)
    ADD_DEPENDENCIES (tpk_root starfish.executable)
ENDIF()
