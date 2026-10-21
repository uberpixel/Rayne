cmake_minimum_required(VERSION 3.28)
project(RayneSARelay)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)

# Keep dependency settings local. Build all libraries with the target toolchain.
block(SCOPE_FOR VARIABLES)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF)
    set(SODIUM_DISABLE_TESTS ON)
    rayne_fetch_content(sarelay_sodium
        GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
        GIT_TAG 9b2848dfc1b917a9410f0de9d81059b26cbfaa8d
        GIT_SUBMODULES libsodium)

    set(OPUS_BUILD_TESTING OFF)
    set(OPUS_BUILD_PROGRAMS OFF)
    set(OPUS_BUILD_SHARED_LIBRARY OFF)
    rayne_fetch_content(sarelay_opus
        GIT_REPOSITORY https://github.com/xiph/opus.git
        GIT_TAG ddbe48383984d56acd9e1ab6a090c54ca6b735a6)

    set(RELAY_SODIUM_TARGET sodium)
    set(RELAY_CLIENT_TESTS OFF)
    rayne_fetch_content(sarelay
        GIT_REPOSITORY https://github.com/SlinDev-GmbH/SlinAir-Relay.git
        GIT_TAG bb2a5bd862dfb5c3385c4eef211906e1cb885a0c
        GIT_SUBMODULES ""
        SOURCE_SUBDIR client)
endblock()

FetchContent_GetProperties(sarelay SOURCE_DIR SARELAY_SOURCE_DIR)

set(SOURCES RNSARelayInit.cpp RNSARelayClient.cpp RNSARelayVoice.cpp)
set(HEADERS RNSARelay.h RNSARelayClient.h RNSARelayVoice.h)
rayne_add_library(RayneSARelay "${SOURCES}" "${HEADERS}" "0.1.0" "1"
    PUBLIC_LIBRARIES Rayne PRIVATE_LIBRARIES slinair-relay-client opus)
rayne_set_module_output_directory(RayneSARelay)
target_compile_definitions(RayneSARelay PRIVATE RN_BUILD_SARELAY)

# Ship the RelayHttp header so all consumers use the same HTTPS contract.
if(IOS OR VISIONOS)
    set_property(TARGET RayneSARelay APPEND PROPERTY PUBLIC_HEADER "${SARELAY_SOURCE_DIR}/client/include/RelayClient.h")
    target_include_directories(RayneSARelay PUBLIC "$<BUILD_INTERFACE:${SARELAY_SOURCE_DIR}/client/include>")
else()
    target_sources(RayneSARelay PUBLIC FILE_SET public_headers TYPE HEADERS
        BASE_DIRS "${SARELAY_SOURCE_DIR}/client/include"
        FILES "${SARELAY_SOURCE_DIR}/client/include/RelayClient.h")
endif()
