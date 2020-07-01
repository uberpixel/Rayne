cmake_minimum_required(VERSION 2.8.4)
project(RayneUI)

include(../../CMake/LibraryTarget.cmake)
include(ExternalProject)

if(APPLE)
        set(SKIA_ARGS
                target_os=\"mac\"
                host_cpu=\"arm64\"
                is_official_build=true
                is_debug=false
                is_component_build=false
                skia_use_fontconfig=false
                skia_use_freetype=true
                skia_enable_atlas_text=false
                skia_use_system_freetype2=false
                skia_use_icu=false
                skia_use_libgifcodec=false
                skia_use_harfbuzz=false
                skia_use_gl=false
                skia_use_expat=false
                skia_enable_gpu=false
                skia_enable_particles=false
                skia_enable_skottie=false
                skia_enable_skshaper=false
                skia_use_system_libpng=false
                skia_use_system_zlib=false
                skia_use_libjpeg_turbo_decode=false
                skia_use_libjpeg_turbo_encode=false
                skia_use_libpng_decode=false
                skia_use_libpng_encode=false
                skia_use_libwebp_decode=false
                skia_use_libwebp_encode=false
                skia_use_lua=false
                skia_use_piex=false
                skia_use_zlib=false
                skia_enable_tools=false
                skia_enable_pdf=false)
elseif(ANDROID)
        set(SKIA_ARGS
                ndk=\"/Users/slin/Library/Android/sdk/ndk-bundle\"
                target_cpu=\"arm64\"
                ndk_api=24
                cc=\"clang\"
                cxx=\"clang++\"
                is_official_build=true
                is_debug=false
                is_component_build=false
                skia_use_freetype=true
                skia_enable_atlas_text=false
                skia_use_system_freetype2=false
                skia_use_system_expat=false
                skia_use_icu=false
                skia_use_system_libpng=false
                skia_use_system_zlib=false
                skia_use_libjpeg_turbo_decode=false
                skia_use_libjpeg_turbo_encode=false
                skia_use_libpng_decode=false
                skia_use_libpng_encode=false
                skia_use_libwebp_decode=false
                skia_use_libwebp_encode=false
                skia_use_lua=false
                skia_use_piex=false
                skia_use_zlib=false
                skia_enable_gpu=false
                skia_use_vulkan=false
                skia_enable_tools=false
                skia_enable_pdf=false)
elseif(UNIX)
        set(SKIA_ARGS
                cc=\"clang\"
                cxx=\"clang++\"
                is_official_build=true
                is_debug=false
                is_component_build=false
                skia_use_fontconfig=false
                skia_use_freetype=true
                skia_enable_atlas_text=false
                skia_use_system_freetype2=false
                skia_use_system_expat=false
                skia_use_icu=false
                skia_use_system_libpng=false
                skia_use_system_zlib=false
                skia_use_libjpeg_turbo_decode=false
                skia_use_libjpeg_turbo_encode=false
                skia_use_libpng_decode=false
                skia_use_libpng_encode=false
                skia_use_libwebp_decode=false
                skia_use_libwebp_encode=false
                skia_use_lua=false
                skia_use_piex=false
                skia_use_zlib=false
                skia_enable_gpu=false
                skia_use_vulkan=false
                skia_enable_tools=false
                skia_enable_pdf=false)
else()
        set(SKIA_ARGS
                extra_cflags=[\"$<IF:$<CONFIG:Debug>,/MDd,/MD>\",\"-DSK_RELEASE\"]
                clang_win=\"C:/Program Files/LLVM\"
                is_official_build=true
                is_debug=false
                is_component_build=false
                skia_use_fontconfig=false
                skia_use_freetype=true
                skia_enable_atlas_text=false
                skia_use_system_freetype2=false
                skia_use_icu=false
                skia_use_system_libpng=false
                skia_use_system_zlib=false
                skia_use_system_expat=false
                skia_use_libjpeg_turbo_decode=false
                skia_use_libjpeg_turbo_encode=false
                skia_use_libpng_decode=false
                skia_use_libpng_encode=false
                skia_use_libwebp_decode=false
                skia_use_libwebp_encode=false
                skia_use_lua=false
                skia_use_piex=false
                skia_use_zlib=false
                skia_enable_gpu=false
                skia_enable_tools=false
                skia_enable_pdf=false)
endif()

list(JOIN SKIA_ARGS " " SKIA_ARGS_STRING)
set(SKIA_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/skia-prefix/src/skia)
set(SKIA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/skia-prefix/src/skia-build)


set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        Sources/RNUIContext.cpp
        Sources/RNUIPath.cpp
        Sources/RNUIView.cpp
        Sources/RNUILabel.cpp
        Sources/RNUIWindow.cpp
        Sources/RNUIServer.cpp
        Sources/RNUIInit.cpp
        Sources/RNUIEdgeInsets.h
        Sources/RNUIImage.cpp
        Sources/RNUIFont.cpp
        Sources/RNUIImageView.cpp
        Sources/RNUIButton.cpp
        Sources/RNUIScrollView.cpp
        Sources/RNUIInternals.h)

set(RESOURCES
        Resources)

set(HEADERS
        Sources/RNUIContext.h
        Sources/RNUIPath.h
        Sources/RNUIView.h
        Sources/RNUILabel.h
        Sources/RNUIWindow.h
        Sources/RNUIServer.h
        Sources/RNUIConfig.h
        Sources/RNUIEdgeInsets.h
        Sources/RNUIImage.h
        Sources/RNUIFont.h
        Sources/RNUIImageView.h
        Sources/RNUIButton.h
        Sources/RNUIScrollView.h
        Sources/RNUI.h)

set(DEFINES
        "RN_BUILD_UI"
        "SK_RELEASE")

if(WIN32)
        set(RAYNE_LIBRARIES Rayne)
elseif(APPLE)
        set(RAYNE_LIBRARIES Rayne ${SKIA_BINARY_DIR}/libskia.a)
elseif(ANDROID)
        set(RAYNE_LIBRARIES Rayne ${SKIA_BINARY_DIR}/libskia.a)
elseif(UNIX)
        set(RAYNE_LIBRARIES Rayne ${SKIA_BINARY_DIR}/libskia.a)
endif()

set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include ${SKIA_SOURCE_DIR} ${SKIA_SOURCE_DIR}/include)

rayne_add_library(RayneUI "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneUI)

ExternalProject_Add(skia
        GIT_REPOSITORY https://skia.googlesource.com/skia.git
        GIT_TAG origin/chrome/m83
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        BUILD_BYPRODUCTS ""
        INSTALL_COMMAND "")

#This one would cause external project to skip all commands after, by splitting this out of externalprojectadd it works...
add_custom_target(skiaupdate
        COMMAND python ${SKIA_SOURCE_DIR}/tools/git-sync-deps
        VERBATIM)

if(APPLE)
        set(SKIA_ARGS_X64 ${SKIA_ARGS} target_cpu=\"x64\" extra_cflags=[\"-mmacosx-version-min=10.12\",\"--target=x86_64-apple-macos10.12\"])
        list(JOIN SKIA_ARGS_X64 " " SKIA_ARGS_X64_STRING)

        set(SKIA_ARGS_ARM64 ${SKIA_ARGS} target_cpu=\"arm64\" extra_cflags=[\"-mmacosx-version-min=10.11\",\"--target=arm64-apple-macos11\"])
        list(JOIN SKIA_ARGS_ARM64 " " SKIA_ARGS_ARM64_STRING)

        add_custom_target(skiabuild
        #        COMMAND sed -i.bak "/opts(\"crc32\")/{n$<SEMICOLON>s/.*/enabled = false/$<SEMICOLON>}" ${SKIA_SOURCE_DIR}/BUILD.gn
                COMMAND ${SKIA_SOURCE_DIR}/bin/gn gen ${SKIA_BINARY_DIR}/x64 --root=${SKIA_SOURCE_DIR} --args=${SKIA_ARGS_X64_STRING}
                COMMAND ninja -C ${SKIA_BINARY_DIR}/x64 skia
                COMMAND ${SKIA_SOURCE_DIR}/bin/gn gen ${SKIA_BINARY_DIR}/arm64 --root=${SKIA_SOURCE_DIR} --args=${SKIA_ARGS_ARM64_STRING}
                COMMAND ninja -C ${SKIA_BINARY_DIR}/arm64 skia
                COMMAND lipo -create -output ${SKIA_BINARY_DIR}/libskia.a ${SKIA_BINARY_DIR}/x64/libskia.a ${SKIA_BINARY_DIR}/arm64/libskia.a
                VERBATIM
                BYPRODUCTS ${SKIA_BINARY_DIR}/libskia.a)
else()
        add_custom_target(skiabuild
                COMMAND ${SKIA_SOURCE_DIR}/bin/gn gen ${SKIA_BINARY_DIR} --root=${SKIA_SOURCE_DIR} --args=${SKIA_ARGS_STRING}
                COMMAND ninja -C ${SKIA_BINARY_DIR} skia
                VERBATIM
                BYPRODUCTS
                        ${SKIA_BINARY_DIR}/libskia.a
                        ${SKIA_BINARY_DIR}/skia.lib)
endif()

add_dependencies(skiaupdate skia)
add_dependencies(skiabuild skiaupdate)
add_dependencies(RayneUI skiabuild)

if(WIN32)
        target_link_libraries(RayneUI ${SKIA_BINARY_DIR}/skia.lib)
endif()

target_include_directories(RayneUI SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneUI PRIVATE "${DEFINES}")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneUI/. DESTINATION lib/Rayne/RayneUI)
