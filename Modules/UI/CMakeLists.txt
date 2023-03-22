cmake_minimum_required(VERSION 3.10.1)
project(RayneUI)

include(../../CMake/LibraryTarget.cmake)
include(ExternalProject)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNUIPath.cpp
        RNUIView.cpp
        RNUIAttributedString.cpp
        RNUILabel.cpp
        RNUIWindow.cpp
        RNUIServer.cpp
        RNUIInit.cpp
        RNUIEdgeInsets.h
        RNUIFontManager.cpp
        RNUIImageView.cpp
        RNUIButton.cpp
        RNUIScrollView.cpp
        RNUISlider.cpp)

set(RESOURCES
        Resources)

set(HEADERS
        RNUIPath.h
        RNUIView.h
        RNUIAttributedString.h
        RNUILabel.h
        RNUIWindow.h
        RNUIServer.h
        RNUIConfig.h
        RNUIEdgeInsets.h
        RNUIFontManager.h
        RNUIImageView.h
        RNUIButton.h
        RNUIScrollView.h
        RNUISlider.h
        RNUI.h)

set(DEFINES
        "RN_BUILD_UI"
        "SK_RELEASE")

set(RAYNE_LIBRARIES Rayne)

set(KALLIGRAPH_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/kalligraph-prefix/src/kalligraph)
set(KALLIGRAPH_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/kalligraph-prefix/src/kalligraph-build)

set(INCLUDE_DIRECTORIES ${Rayne_BINARY_DIR}/include ${KALLIGRAPH_SOURCE_DIR}/Sources ${CMAKE_CURRENT_SOURCE_DIR}/Vendor)

rayne_add_library(RayneUI "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneUI)

ExternalProject_Add(Kalligraph
        GIT_REPOSITORY https://github.com/Slin/Kalligraph.git
        GIT_TAG origin/master
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        CMAKE_CACHE_ARGS
                "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH:BOOL=False"
                "-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}"
                "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
                "-DANDROID_NATIVE_API_LEVEL:STRING=${ANDROID_NATIVE_API_LEVEL}"
                "-DANDROID_ABI:STRING=${ANDROID_ABI}"
                "-DANDROID_NDK:STRING=${ANDROID_NDK}"
                "-DANDROID_STL:STRING=${ANDROID_STL}"
                "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
        BUILD_BYPRODUCTS
                ${KALLIGRAPH_BINARY_DIR}/libKalligraph.a
                ${KALLIGRAPH_BINARY_DIR}/Debug/libKalligraph.a
                ${KALLIGRAPH_BINARY_DIR}/Release/libKalligraph.a
                ${KALLIGRAPH_BINARY_DIR}/Debug/Kalligraph.lib
                ${KALLIGRAPH_BINARY_DIR}/Release/Kalligraph.lib
        INSTALL_COMMAND ""
        UPDATE_COMMAND "")

add_library(KalligraphLib STATIC IMPORTED)

if(WIN32)
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")

        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION_DEBUG "${KALLIGRAPH_BINARY_DIR}/Debug/Kalligraph.lib")
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION_RELEASE "${KALLIGRAPH_BINARY_DIR}/Release/Kalligraph.lib")
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${KALLIGRAPH_BINARY_DIR}/RelWithDebInfo/Kalligraph.lib")

elseif(ANDROID)
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION "${KALLIGRAPH_BINARY_DIR}/libKalligraph.a")
else()
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")

        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION_DEBUG "${KALLIGRAPH_BINARY_DIR}/Debug/libKalligraph.a")
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION_RELEASE "${KALLIGRAPH_BINARY_DIR}/Release/libKalligraph.a")
        SET_PROPERTY(TARGET KalligraphLib PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO "${KALLIGRAPH_BINARY_DIR}/RelWithDebInfo/libKalligraph.a")
endif()

add_dependencies(KalligraphLib Kalligraph)
target_link_libraries(RayneUI PUBLIC KalligraphLib)

target_include_directories(RayneUI SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneUI PRIVATE "${DEFINES}")


rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneUI/. DESTINATION lib/Rayne/RayneUI)
