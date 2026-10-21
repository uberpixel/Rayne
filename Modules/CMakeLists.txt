cmake_minimum_required(VERSION 3.28)
project(Rayne-Modules)

option(RN_BUILD_METAL_MODULE "Build the Metal renderer module" ON)
option(RN_BUILD_VULKAN_MODULE "Build the Vulkan renderer module" ON)
option(RN_BUILD_ASSIMP_MODULE "Build the Assimp module" OFF)
option(RN_BUILD_GLTF_MODULE "Build the glTF module" OFF)
option(RN_BUILD_OPENXR_MODULE "Build the OpenXR module" OFF)
option(RN_BUILD_APPLEXR_MODULE "Build the AppleXR module" OFF)
option(RN_BUILD_VRSTUB_MODULE "Build the VRStub module" OFF)
option(RN_BUILD_BULLET_MODULE "Build the Bullet physics module" ON)
option(RN_BUILD_PHYSX_MODULE "Build the PhysX physics module" OFF)
option(RN_BUILD_JOLT_MODULE "Build the Jolt physics module" OFF)
option(RN_BUILD_SPLASH_MODULE "Build the Splash physics module" OFF)
option(RN_BUILD_ODE_MODULE "Build the ODE physics module" OFF)
option(RN_BUILD_NEWTON_MODULE "Build the Newton Game Dynamics physics module" OFF)
option(RN_BUILD_OGG_MODULE "Build the ogg audio file module" ON)
option(RN_BUILD_OPENAL_MODULE "Build the OpenAL audio module" ON)
option(RN_BUILD_STEAMAUDIO_MODULE "Build the Steam Audio module" OFF)
option(RN_BUILD_OCULUSAUDIO_MODULE "Build the Oculus Audio module" OFF)
option(RN_BUILD_RESONANCEAUDIO_MODULE "Build the Resonance Audio module" OFF)
option(RN_BUILD_ENET_MODULE "Build the ENet module" OFF)
option(RN_BUILD_SARELAY_MODULE "Build the SlinAir Relay networking and voice module" OFF)
option(RN_BUILD_RECAST_MODULE "Build the Recast navmesh module" OFF)
option(RN_BUILD_UI_MODULE "Build the UI module" ON)
option(RN_BUILD_FLEXUI_MODULE "Build the FlexUI module" ON)
option(RN_BUILD_EPICONLINESERVICES_MODULE "Build the Epic Online Services module" OFF)
option(RN_BUILD_CAPTURE_MODULE "Build the Capture module" OFF)
option(RN_BUILD_BHAPTICS_MODULE "Build the bHaptics module" OFF)

if(RAYNE_UNIVERSE_SCALE)
    set(RN_UNIVERSE_SCALE_UNSUPPORTED_MODULE_OPTIONS
        RN_BUILD_BULLET_MODULE
        RN_BUILD_NEWTON_MODULE
        RN_BUILD_ODE_MODULE
        RN_BUILD_PHYSX_MODULE
        RN_BUILD_RECAST_MODULE
        RN_BUILD_SPLASH_MODULE
        RN_BUILD_STEAMAUDIO_MODULE
        RN_BUILD_OCULUSAUDIO_MODULE)

    foreach(MODULE_OPTION IN LISTS RN_UNIVERSE_SCALE_UNSUPPORTED_MODULE_OPTIONS)
        if(${MODULE_OPTION})
            message(FATAL_ERROR "${MODULE_OPTION} does not support RAYNE_UNIVERSE_SCALE")
        endif()
    endforeach()
endif()

if(APPLE AND ${RN_BUILD_METAL_MODULE})
    add_subdirectory("Metal")
endif()
if(NOT APPLE AND ${RN_BUILD_VULKAN_MODULE})
    add_subdirectory("Vulkan")
endif()

if(${RN_BUILD_ASSIMP_MODULE})
    add_subdirectory("Assimp")
endif()

if(${RN_BUILD_GLTF_MODULE})
    add_subdirectory("glTF")
endif()

if(${RN_BUILD_BULLET_MODULE})
    add_subdirectory("Bullet")
endif()

if(${RN_BUILD_PHYSX_MODULE})
    add_subdirectory("PhysX")
endif()

if(${RN_BUILD_JOLT_MODULE})
    add_subdirectory("Jolt")
endif()

if(${RN_BUILD_SPLASH_MODULE})
    add_subdirectory("Splash")
endif()

if(${RN_BUILD_ODE_MODULE})
    add_subdirectory("ODE")
endif()

if(${RN_BUILD_NEWTON_MODULE})
    add_subdirectory("Newton")
endif()

if(${RN_BUILD_OGG_MODULE})
    add_subdirectory("ogg")
endif()

if(${RN_BUILD_OPENAL_MODULE})
    add_subdirectory("OpenAL")
endif()

if(${RN_BUILD_STEAMAUDIO_MODULE})
    add_subdirectory("SteamAudio")
endif()

if(${RN_BUILD_OCULUSAUDIO_MODULE})
    add_subdirectory("OculusAudio")
endif()

if(${RN_BUILD_RESONANCEAUDIO_MODULE})
    add_subdirectory("ResonanceAudio")
endif()

if(${RN_BUILD_ENET_MODULE})
    add_subdirectory("ENet")
endif()

if(RN_BUILD_SARELAY_MODULE)
    add_subdirectory("SARelay")
endif()

if(${RN_BUILD_RECAST_MODULE})
    add_subdirectory("Recast")
endif()

if(${RN_BUILD_UI_MODULE})
    add_subdirectory("UI")
endif()

if(${RN_BUILD_FLEXUI_MODULE})
    add_subdirectory("FlexUI")
endif()

if(${RN_BUILD_EPICONLINESERVICES_MODULE})
    add_subdirectory("EpicOnlineServices")
endif()

if(${RN_BUILD_VRSTUB_MODULE} OR (NOT VISIONOS AND NOT IOS AND ${RN_BUILD_OPENXR_MODULE}) OR RN_BUILD_APPLEXR_MODULE)
	add_subdirectory("VRWrapper")
endif()

if(NOT VISIONOS AND NOT IOS AND ${RN_BUILD_OPENXR_MODULE})
    add_subdirectory("OpenXR")
endif()

if(VISIONOS AND ${RN_BUILD_APPLEXR_MODULE})
    add_subdirectory("AppleXR")
endif()

if(${RN_BUILD_BHAPTICS_MODULE})
    add_subdirectory("bHaptics")
endif()

if(RN_BUILD_CAPTURE_MODULE)
    add_subdirectory("Capture")
endif()
