#######################################################
# PLATFORM CRYPTO (src/platform/crypto)
#######################################################
# The OpenSSL layer of WebCrypto. It is a separate static library, like the
# CLI, so that it does not see the Starfish include directories. The build
# then fails if this layer starts to depend on Starfish code. See
# src/platform/crypto/PlatformCrypto.h.

SET (_crypto_sources
    ${STARFISH_ROOT}/src/platform/crypto/PlatformCrypto.cpp
    ${STARFISH_ROOT}/src/platform/crypto/PlatformCryptoAsymmetric.cpp
)
ADD_LIBRARY (starfish.crypto STATIC ${_crypto_sources})
UNSET (_crypto_sources)

# Use the OpenSSL that Starfish builds or finds for this platform. Tizen 10.0
# and below have OpenSSL 1.1, so the code uses APIs that 1.1.1 and 3 share.
IF (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND OPENSSL_BUILD_PATH)
    SET (_crypto_openssl_include ${OPENSSL_BUILD_PATH}/source/include)
ELSE ()
    SET (_crypto_openssl_include ${STARFISH_OPENSSL_INCLUDEDIR})
ENDIF ()

# Consumers include PlatformCrypto.h by name. The header uses only the C++
# standard library, so OpenSSL stays PRIVATE.
TARGET_INCLUDE_DIRECTORIES (starfish.crypto
    PUBLIC ${STARFISH_ROOT}/src/platform/crypto
    PRIVATE ${_crypto_openssl_include}
)
UNSET (_crypto_openssl_include)
TARGET_COMPILE_OPTIONS (starfish.crypto PRIVATE ${LWE_CXXFLAGS})
# PlatformCrypto.h uses std::vector, so the STL mode must match the Starfish
# library. A Debug build sets _GLIBCXX_DEBUG, which changes std::vector.
# PUBLIC gives the same defines to the unit test.
TARGET_COMPILE_DEFINITIONS (starfish.crypto PUBLIC ${LWE_DEFINES_MODE})
# Linked by name, like STARFISH_LIBRARIES_DEFAULT does.
TARGET_LINK_LIBRARIES (starfish.crypto PRIVATE crypto)
# OpenSSL is a custom target. Its headers must exist before this compiles.
IF (TARGET openssl)
    ADD_DEPENDENCIES (starfish.crypto openssl)
ENDIF ()

# The Starfish library uses this layer through src/core/modules/crypto.
TARGET_LINK_LIBRARIES (starfish.shared_library starfish.crypto)
TARGET_LINK_LIBRARIES (starfish.static_library starfish.crypto)

IF (TARGET gtest)
    ADD_EXECUTABLE (starfish.crypto.unit_test EXCLUDE_FROM_ALL
        ${STARFISH_ROOT}/src/platform/crypto/test/PlatformCryptoTest.cpp
        ${STARFISH_ROOT}/src/platform/crypto/test/PlatformCryptoAsymmetricTest.cpp
    )
    TARGET_LINK_LIBRARIES (starfish.crypto.unit_test
        starfish.crypto gtest gtest_main crypto -lpthread
    )
    # The test checks the OpenSSL error queue, so it needs the headers too.
    TARGET_INCLUDE_DIRECTORIES (starfish.crypto.unit_test PRIVATE
        $<TARGET_PROPERTY:starfish.crypto,INCLUDE_DIRECTORIES>
    )
    TARGET_COMPILE_OPTIONS (starfish.crypto.unit_test PRIVATE ${LWE_CXXFLAGS})
    SET_TARGET_PROPERTIES (starfish.crypto.unit_test PROPERTIES
        OUTPUT_NAME "${TARGETNAME}-crypto-unit-test"
    )
ENDIF ()
