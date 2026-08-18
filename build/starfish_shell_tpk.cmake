CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

SET(SHELL_PACKAGE_ID "org.tizen.lightweight-web-engine-shell")
SET(TIZEN_VERSION "${TIZEN_MAJOR_VERSION}.${TIZEN_MINOR_VERSION}")
SET(SHELL_TPK_ROOT ${OUTPUT_DIRECTORY}/shell_tpk_root)
SET(SHELL_TPK_VERSION 0.0.1)
SET(SHELL_TPK_NAME ${SHELL_PACKAGE_ID}-${SHELL_TPK_VERSION}.${CMAKE_SYSTEM_PROCESSOR}.tpk)
SET(HASH_SIGNER_SH "/usr/bin/hash-signer.sh")
GET_TARGET_PROPERTY(STARFISH_API_OUTPUT_NAME starfish_api.shared_library OUTPUT_NAME)

CONFIGURE_FILE(build/tizen/tpk/shell_tizen-manifest.xml.in ${OUTPUT_DIRECTORY}/shell_tizen-manifest.xml)

ADD_CUSTOM_TARGET (shell_tpk_root
    COMMAND echo "Make tpk root..."
    COMMAND rm -rf ${SHELL_TPK_ROOT}
    COMMAND install -d ${SHELL_TPK_ROOT}
    COMMAND install -m 0644 ${OUTPUT_DIRECTORY}/shell_tizen-manifest.xml ${SHELL_TPK_ROOT}/tizen-manifest.xml
    COMMAND install -d ${SHELL_TPK_ROOT}/bin
    COMMAND install -m 0755 ${OUTPUT_DIRECTORY}/${TARGETNAME} ${SHELL_TPK_ROOT}/bin/
    COMMAND install -d ${SHELL_TPK_ROOT}/lib/
    COMMAND install -m 0644 ${OUTPUT_DIRECTORY}/lib/*.so* ${SHELL_TPK_ROOT}/lib/
    COMMAND strip -v --strip-all ${SHELL_TPK_ROOT}/lib/*.so
    COMMAND mv ${SHELL_TPK_ROOT}/lib/lib${TARGETNAME}.so ${SHELL_TPK_ROOT}/lib/liblightweight-web-engine.so.1
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(kuep_signed_files_shell
    COMMAND echo "Sign files using kuep_signer..."
    COMMAND kuep_signer.sh -tizen_major_ver ${TIZEN_MAJOR_VERSION} ${SHELL_TPK_ROOT}/bin/${TARGETNAME}
    COMMAND find ${SHELL_TPK_ROOT}/lib/ -name *.so* -exec kuep_signer.sh -tizen_major_ver ${TIZEN_MAJOR_VERSION} {} "\\;"
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(signed_shell_tpk_root
    COMMAND echo "Sign tpk_root using hash-signer..."
    COMMAND ${HASH_SIGNER_SH} -a -d -p platform ${SHELL_TPK_ROOT}
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(shell_tpk
    COMMAND echo "Packge tpk_root.."
    COMMAND rm -f ${OUTPUT_DIRECTORY}/${SHELL_TPK_NAME}
    COMMAND pushd ${SHELL_TPK_ROOT}
    COMMAND zip -yr ${SHELL_TPK_NAME} *
    COMMAND mv ${SHELL_TPK_NAME} ../
    COMMAND popd
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET(resigned_shell_tpk
    COMMAND echo "Resign tpk using tpkresigner..."
    COMMAND tpkresigner -a -d -p platform -n ${SHELL_PACKAGE_ID} ${OUTPUT_DIRECTORY}/${SHELL_TPK_NAME}
    COMMAND echo "Done."
)

ADD_CUSTOM_TARGET (starfish.executable.tpk
    COMMENT "starfish.executable.tpk TARGET"
)

IF (${CUSTOM} STREQUAL "prod_tv")
    ADD_DEPENDENCIES (starfish.executable.tpk resigned_shell_tpk)
    ADD_DEPENDENCIES (resigned_shell_tpk shell_tpk)
    ADD_DEPENDENCIES (shell_tpk signed_shell_tpk_root)
    ADD_DEPENDENCIES (signed_shell_tpk_root kuep_signed_files_shell)
    ADD_DEPENDENCIES (kuep_signed_files_shell shell_tpk_root)
    ADD_DEPENDENCIES (shell_tpk_root starfish.executable)
ELSE()
    ADD_DEPENDENCIES (starfish.executable.tpk shell_tpk)
    ADD_DEPENDENCIES (shell_tpk signed_shell_tpk_root)
    ADD_DEPENDENCIES (signed_shell_tpk_root shell_tpk_root)
    ADD_DEPENDENCIES (shell_tpk_root starfish.executable)
ENDIF()
