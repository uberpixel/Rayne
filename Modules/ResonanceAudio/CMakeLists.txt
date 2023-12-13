cmake_minimum_required(VERSION 3.10.1)
project(RayneResonanceAudio)

include(../../CMake/LibraryTarget.cmake)
include(../../CMake/Rayne.cmake)
include(ExternalProject)

#Resonance does the audio simulation
FetchContent_Declare(
        resonance
        GIT_REPOSITORY "https://github.com/Slin/resonance-audio.git"
        GIT_SHALLOW ON
        PATCH_COMMAND "${CMAKE_BINARY_DIR}/_deps/resonance-src/third_party/clone_core_deps.sh"
)
#Use this to setup a custom target and don't actually include the directory in the project
FetchContent_GetProperties(resonance)
if(NOT resonance_POPULATED)
  FetchContent_Populate(resonance)
endif()

#Resonance build files don't really work with xcode, so I am just setting things up myself here
set(RESONANCE_SOURCE_DIR ${resonance_SOURCE_DIR}/resonance_audio/)
set(RESONANCE_PFFFT_DIR "${resonance_SOURCE_DIR}/third_party/pffft/" CACHE PATH "Path to pffft library")
set(RESONANCE_PFFFT_INCLUDE_DIR ${RESONANCE_PFFFT_DIR})
set(RESONANCE_SADIE_HRTFS_DIR "${resonance_SOURCE_DIR}/third_party/SADIE_hrtf_database/generated/" CACHE PATH "Path to SADIE_hrtf_database library")

set(RESONANCE_EIGEN3_DIR "${resonance_SOURCE_DIR}/third_party/eigen/" CACHE PATH "Path to eigen3 library")
set(RESONANCE_EIGEN3_INCLUDE_DIR ${RESONANCE_EIGEN3_DIR})
add_definitions(-DEIGEN_MPL2_ONLY)

# Build Resonance Audio.
set(RESONANCE_SOURCES
        ${RESONANCE_PFFFT_DIR}/fftpack.c
        ${RESONANCE_PFFFT_DIR}/fftpack.h
        ${RESONANCE_PFFFT_DIR}/pffft.c
        ${RESONANCE_PFFFT_DIR}/pffft.h

        ${RESONANCE_SADIE_HRTFS_DIR}/hrtf_assets.cc
        ${RESONANCE_SADIE_HRTFS_DIR}/hrtf_assets.h

        ${resonance_SOURCE_DIR}/platforms/common/utils.cc
        ${resonance_SOURCE_DIR}/platforms/common/utils.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_binaural_decoder.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_binaural_decoder.cc
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_codec.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_codec_impl.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_lookup_table.cc
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_lookup_table.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/ambisonic_spread_coefficients.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/associated_legendre_polynomials_generator.cc
        ${RESONANCE_SOURCE_DIR}/ambisonics/associated_legendre_polynomials_generator.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/foa_rotator.cc
        ${RESONANCE_SOURCE_DIR}/ambisonics/foa_rotator.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/hoa_rotator.cc
        ${RESONANCE_SOURCE_DIR}/ambisonics/hoa_rotator.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/stereo_from_soundfield_converter.cc
        ${RESONANCE_SOURCE_DIR}/ambisonics/stereo_from_soundfield_converter.h
        ${RESONANCE_SOURCE_DIR}/ambisonics/utils.h
        ${RESONANCE_SOURCE_DIR}/api/binaural_surround_renderer.cc
        ${RESONANCE_SOURCE_DIR}/api/binaural_surround_renderer.h
        ${RESONANCE_SOURCE_DIR}/api/resonance_audio_api.cc
        ${RESONANCE_SOURCE_DIR}/api/resonance_audio_api.h
        ${RESONANCE_SOURCE_DIR}/base/aligned_allocator.h
        ${RESONANCE_SOURCE_DIR}/base/audio_buffer.cc
        ${RESONANCE_SOURCE_DIR}/base/audio_buffer.h
        ${RESONANCE_SOURCE_DIR}/base/channel_view.cc
        ${RESONANCE_SOURCE_DIR}/base/channel_view.h
        ${RESONANCE_SOURCE_DIR}/base/constants_and_types.h
        ${RESONANCE_SOURCE_DIR}/base/integral_types.h
        ${RESONANCE_SOURCE_DIR}/base/logging.h
        ${RESONANCE_SOURCE_DIR}/base/misc_math.cc
        ${RESONANCE_SOURCE_DIR}/base/misc_math.h
        ${RESONANCE_SOURCE_DIR}/base/object_transform.h
        ${RESONANCE_SOURCE_DIR}/base/simd_macros.h
        ${RESONANCE_SOURCE_DIR}/base/simd_utils.cc
        ${RESONANCE_SOURCE_DIR}/base/simd_utils.h
        ${RESONANCE_SOURCE_DIR}/base/source_parameters.h
        ${RESONANCE_SOURCE_DIR}/base/spherical_angle.cc
        ${RESONANCE_SOURCE_DIR}/base/spherical_angle.h
        ${RESONANCE_SOURCE_DIR}/base/unique_ptr_wrapper.h
        ${RESONANCE_SOURCE_DIR}/config/global_config.h
        ${RESONANCE_SOURCE_DIR}/config/source_config.cc
        ${RESONANCE_SOURCE_DIR}/config/source_config.h
        ${RESONANCE_SOURCE_DIR}/dsp/biquad_filter.cc
        ${RESONANCE_SOURCE_DIR}/dsp/biquad_filter.h
        ${RESONANCE_SOURCE_DIR}/dsp/channel_converter.cc
        ${RESONANCE_SOURCE_DIR}/dsp/channel_converter.h
        ${RESONANCE_SOURCE_DIR}/dsp/circular_buffer.cc
        ${RESONANCE_SOURCE_DIR}/dsp/circular_buffer.h
        ${RESONANCE_SOURCE_DIR}/dsp/delay_filter.cc
        ${RESONANCE_SOURCE_DIR}/dsp/delay_filter.h
        ${RESONANCE_SOURCE_DIR}/dsp/distance_attenuation.cc
        ${RESONANCE_SOURCE_DIR}/dsp/distance_attenuation.h
        ${RESONANCE_SOURCE_DIR}/dsp/fft_manager.cc
        ${RESONANCE_SOURCE_DIR}/dsp/fft_manager.h
        ${RESONANCE_SOURCE_DIR}/dsp/filter_coefficient_generators.cc
        ${RESONANCE_SOURCE_DIR}/dsp/filter_coefficient_generators.h
        ${RESONANCE_SOURCE_DIR}/dsp/fir_filter.cc
        ${RESONANCE_SOURCE_DIR}/dsp/fir_filter.h
        ${RESONANCE_SOURCE_DIR}/dsp/gain.cc
        ${RESONANCE_SOURCE_DIR}/dsp/gain.h
        ${RESONANCE_SOURCE_DIR}/dsp/gain_mixer.cc
        ${RESONANCE_SOURCE_DIR}/dsp/gain_mixer.h
        ${RESONANCE_SOURCE_DIR}/dsp/gain_processor.cc
        ${RESONANCE_SOURCE_DIR}/dsp/gain_processor.h
        ${RESONANCE_SOURCE_DIR}/dsp/mixer.cc
        ${RESONANCE_SOURCE_DIR}/dsp/mixer.h
        ${RESONANCE_SOURCE_DIR}/dsp/mono_pole_filter.cc
        ${RESONANCE_SOURCE_DIR}/dsp/mono_pole_filter.h
        ${RESONANCE_SOURCE_DIR}/dsp/multi_channel_iir.cc
        ${RESONANCE_SOURCE_DIR}/dsp/multi_channel_iir.h
        ${RESONANCE_SOURCE_DIR}/dsp/near_field_processor.cc
        ${RESONANCE_SOURCE_DIR}/dsp/near_field_processor.h
        ${RESONANCE_SOURCE_DIR}/dsp/occlusion_calculator.cc
        ${RESONANCE_SOURCE_DIR}/dsp/occlusion_calculator.h
        ${RESONANCE_SOURCE_DIR}/dsp/partitioned_fft_filter.cc
        ${RESONANCE_SOURCE_DIR}/dsp/partitioned_fft_filter.h
        ${RESONANCE_SOURCE_DIR}/dsp/reflection.h
        ${RESONANCE_SOURCE_DIR}/dsp/reflections_processor.cc
        ${RESONANCE_SOURCE_DIR}/dsp/reflections_processor.h
        ${RESONANCE_SOURCE_DIR}/dsp/resampler.cc
        ${RESONANCE_SOURCE_DIR}/dsp/resampler.h
        ${RESONANCE_SOURCE_DIR}/dsp/reverb_onset_compensator.cc
        ${RESONANCE_SOURCE_DIR}/dsp/reverb_onset_compensator.h
        ${RESONANCE_SOURCE_DIR}/dsp/reverb_onset_update_processor.cc
        ${RESONANCE_SOURCE_DIR}/dsp/reverb_onset_update_processor.h
        ${RESONANCE_SOURCE_DIR}/dsp/sh_hrir_creator.cc
        ${RESONANCE_SOURCE_DIR}/dsp/sh_hrir_creator.h
        ${RESONANCE_SOURCE_DIR}/dsp/shoe_box_room.cc
        ${RESONANCE_SOURCE_DIR}/dsp/shoe_box_room.h
        ${RESONANCE_SOURCE_DIR}/dsp/spectral_reverb.cc
        ${RESONANCE_SOURCE_DIR}/dsp/spectral_reverb.h
        ${RESONANCE_SOURCE_DIR}/dsp/spectral_reverb_constants_and_tables.h
        ${RESONANCE_SOURCE_DIR}/dsp/stereo_panner.cc
        ${RESONANCE_SOURCE_DIR}/dsp/stereo_panner.h
        ${RESONANCE_SOURCE_DIR}/dsp/utils.cc
        ${RESONANCE_SOURCE_DIR}/dsp/utils.h
        ${RESONANCE_SOURCE_DIR}/graph/ambisonic_binaural_decoder_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/ambisonic_binaural_decoder_node.h
        ${RESONANCE_SOURCE_DIR}/graph/ambisonic_mixing_encoder_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/ambisonic_mixing_encoder_node.h
        ${RESONANCE_SOURCE_DIR}/graph/binaural_surround_renderer_impl.cc
        ${RESONANCE_SOURCE_DIR}/graph/binaural_surround_renderer_impl.h
        ${RESONANCE_SOURCE_DIR}/graph/buffered_source_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/buffered_source_node.h
        ${RESONANCE_SOURCE_DIR}/graph/foa_rotator_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/foa_rotator_node.h
        ${RESONANCE_SOURCE_DIR}/graph/gain_mixer_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/gain_mixer_node.h
        ${RESONANCE_SOURCE_DIR}/graph/gain_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/gain_node.h
        ${RESONANCE_SOURCE_DIR}/graph/graph_manager.cc
        ${RESONANCE_SOURCE_DIR}/graph/graph_manager.h
        ${RESONANCE_SOURCE_DIR}/graph/graph_manager_config.h
        ${RESONANCE_SOURCE_DIR}/graph/hoa_rotator_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/hoa_rotator_node.h
        ${RESONANCE_SOURCE_DIR}/graph/mixer_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/mixer_node.h
        ${RESONANCE_SOURCE_DIR}/graph/mono_from_soundfield_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/mono_from_soundfield_node.h
        ${RESONANCE_SOURCE_DIR}/graph/near_field_effect_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/near_field_effect_node.h
        ${RESONANCE_SOURCE_DIR}/graph/occlusion_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/occlusion_node.h
        ${RESONANCE_SOURCE_DIR}/graph/reflections_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/reflections_node.h
        ${RESONANCE_SOURCE_DIR}/graph/resonance_audio_api_impl.cc
        ${RESONANCE_SOURCE_DIR}/graph/resonance_audio_api_impl.h
        ${RESONANCE_SOURCE_DIR}/graph/reverb_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/reverb_node.h
        ${RESONANCE_SOURCE_DIR}/graph/source_graph_config.h
        ${RESONANCE_SOURCE_DIR}/graph/source_parameters_manager.cc
        ${RESONANCE_SOURCE_DIR}/graph/source_parameters_manager.h
        ${RESONANCE_SOURCE_DIR}/graph/stereo_mixing_panner_node.cc
        ${RESONANCE_SOURCE_DIR}/graph/stereo_mixing_panner_node.h
        ${RESONANCE_SOURCE_DIR}/graph/system_settings.h
        ${RESONANCE_SOURCE_DIR}/node/node.h
        ${RESONANCE_SOURCE_DIR}/node/processing_node.cc
        ${RESONANCE_SOURCE_DIR}/node/processing_node.h
        ${RESONANCE_SOURCE_DIR}/node/publisher_node.h
        ${RESONANCE_SOURCE_DIR}/node/sink_node.cc
        ${RESONANCE_SOURCE_DIR}/node/sink_node.h
        ${RESONANCE_SOURCE_DIR}/node/source_node.cc
        ${RESONANCE_SOURCE_DIR}/node/source_node.h
        ${RESONANCE_SOURCE_DIR}/node/subscriber_node.h
        ${resonance_SOURCE_DIR}/platforms/common/room_effects_utils.cc
        ${resonance_SOURCE_DIR}/platforms/common/room_effects_utils.h
        ${RESONANCE_SOURCE_DIR}/utils/buffer_crossfader.cc
        ${RESONANCE_SOURCE_DIR}/utils/buffer_crossfader.h
        ${RESONANCE_SOURCE_DIR}/utils/buffer_partitioner.cc
        ${RESONANCE_SOURCE_DIR}/utils/buffer_partitioner.h
        ${RESONANCE_SOURCE_DIR}/utils/buffer_unpartitioner.cc
        ${RESONANCE_SOURCE_DIR}/utils/buffer_unpartitioner.h
        ${RESONANCE_SOURCE_DIR}/utils/lockless_task_queue.cc
        ${RESONANCE_SOURCE_DIR}/utils/lockless_task_queue.h
        ${RESONANCE_SOURCE_DIR}/utils/planar_interleaved_conversion.cc
        ${RESONANCE_SOURCE_DIR}/utils/planar_interleaved_conversion.h
        ${RESONANCE_SOURCE_DIR}/utils/pseudoinverse.h
        ${RESONANCE_SOURCE_DIR}/utils/sample_type_conversion.cc
        ${RESONANCE_SOURCE_DIR}/utils/sample_type_conversion.h
        ${RESONANCE_SOURCE_DIR}/utils/semi_lockless_fifo.h
        ${RESONANCE_SOURCE_DIR}/utils/sum_and_difference_processor.cc
        ${RESONANCE_SOURCE_DIR}/utils/sum_and_difference_processor.h
        ${RESONANCE_SOURCE_DIR}/utils/threadsafe_fifo.h
        ${RESONANCE_SOURCE_DIR}/utils/wav.cc
        ${RESONANCE_SOURCE_DIR}/utils/wav.h
        ${RESONANCE_SOURCE_DIR}/utils/wav_reader.cc
        ${RESONANCE_SOURCE_DIR}/utils/wav_reader.h
        )

add_library(ResonanceAudioStaticCustom STATIC ${RESONANCE_SOURCES})
target_include_directories(ResonanceAudioStaticCustom PRIVATE ${resonance_SOURCE_DIR}/)
target_include_directories(ResonanceAudioStaticCustom PRIVATE ${resonance_SOURCE_DIR}/resonance_audio/)
target_include_directories(ResonanceAudioStaticCustom PRIVATE ${RESONANCE_EIGEN3_INCLUDE_DIR}/)
target_include_directories(ResonanceAudioStaticCustom PRIVATE ${RESONANCE_PFFFT_INCLUDE_DIR}/)


#Miniaudio is a single header library that offers low level multi platform audio in and output
ExternalProject_Add(miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/miniaudio-prefix/src/miniaudio/miniaudio.h"
        INSTALL_COMMAND "")

set(VERSION_MAJOR 0)
set(VERSION_MINOR 5)
set(VERSION_PATCH 0)
set(VERSION_ABI 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(SOURCES
        RNResonanceAudioInit.cpp
        RNResonanceAudioWorld.cpp
        RNResonanceAudioSystem.cpp
        RNResonanceAudioSampler.cpp
        RNResonanceAudioSource.cpp
        RNResonanceAudioInternals.cpp)

set(HEADERS
	RNResonanceAudio.h
        RNResonanceAudioWorld.h
        RNResonanceAudioSystem.h
        RNResonanceAudioSampler.h
        RNResonanceAudioSource.h
        RNResonanceAudioInternals.h)

set(DEFINES RN_BUILD_OCULUS_AUDIO)
set(INCLUDE_DIRECTORIES "${Rayne_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/ResonanceAudio/Include" "${CMAKE_CURRENT_BINARY_DIR}/miniaudio-prefix/src/miniaudio" "${resonance_SOURCE_DIR}/resonance_audio" "${resonance_SOURCE_DIR}" "${resonance_SOURCE_DIR}/third_party/eigen")

if(APPLE)
        find_library(COREAUDIO_LIB CoreAudio)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        set(RAYNE_LIBRARIES Rayne ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
elseif(WIN32)
        #set(RAYNE_LIBRARIES Rayne winmm ole32)
endif()

rayne_add_library(RayneResonanceAudio "${SOURCES}" "${HEADERS}" "${RAYNE_LIBRARIES}" "${VERSION_STRING}" "${VERSION_ABI}")
rayne_set_module_output_directory(RayneResonanceAudio)

target_include_directories(RayneResonanceAudio SYSTEM PRIVATE "${INCLUDE_DIRECTORIES}")
target_compile_definitions(RayneResonanceAudio PRIVATE "${DEFINES}")

add_dependencies(RayneResonanceAudio miniaudio)
target_link_libraries(RayneResonanceAudio PRIVATE ResonanceAudioStaticCustom)

rayne_install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../Build/RayneResonanceAudio/. DESTINATION lib/Rayne/RayneResonanceAudio)

