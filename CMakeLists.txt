cmake_minimum_required(VERSION 3.24)

project(PratStrike VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(MACOSX_BUNDLE FALSE)

include(FetchContent)

set(SDL_SHARED ON CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Disable SDL static library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Disable SDL tests" FORCE)

set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "Disable SDL_image samples" FORCE)
set(SDL2IMAGE_TESTS OFF CACHE BOOL "Disable SDL_image tests" FORCE)
set(SDL2IMAGE_VENDORED ON CACHE BOOL "Use vendored dependencies for SDL_image" FORCE)
set(SDL2IMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "Disable Apple image backend for consistent PNG path" FORCE)
set(SDL2IMAGE_PNG ON CACHE BOOL "Enable PNG support" FORCE)
set(SDL2IMAGE_JPG OFF CACHE BOOL "Disable JPG support" FORCE)
set(SDL2IMAGE_TIF OFF CACHE BOOL "Disable TIF support" FORCE)
set(SDL2IMAGE_WEBP OFF CACHE BOOL "Disable WEBP support" FORCE)
set(SDL2IMAGE_AVIF OFF CACHE BOOL "Disable AVIF support" FORCE)
set(SDL2IMAGE_BMP OFF CACHE BOOL "Disable BMP support" FORCE)
set(SDL2IMAGE_GIF OFF CACHE BOOL "Disable GIF support" FORCE)
set(SDL2IMAGE_JXL OFF CACHE BOOL "Disable JXL support" FORCE)
set(SDL2IMAGE_LBM OFF CACHE BOOL "Disable LBM support" FORCE)
set(SDL2IMAGE_PCX OFF CACHE BOOL "Disable PCX support" FORCE)
set(SDL2IMAGE_PNM OFF CACHE BOOL "Disable PNM support" FORCE)
set(SDL2IMAGE_QOI OFF CACHE BOOL "Disable QOI support" FORCE)
set(SDL2IMAGE_SVG OFF CACHE BOOL "Disable SVG support" FORCE)
set(SDL2IMAGE_XCF OFF CACHE BOOL "Disable XCF support" FORCE)
set(SDL2IMAGE_XPM OFF CACHE BOOL "Disable XPM support" FORCE)
set(SDL2IMAGE_XV OFF CACHE BOOL "Disable XV support" FORCE)

set(SDL2MIXER_SAMPLES OFF CACHE BOOL "Disable SDL_mixer samples" FORCE)
set(SDL2MIXER_VENDORED ON CACHE BOOL "Use vendored dependencies for SDL_mixer" FORCE)
set(SDL2MIXER_VORBIS STB CACHE STRING "Use stb_vorbis for OGG support" FORCE)
set(SDL2MIXER_WAVE ON CACHE BOOL "Enable WAV support" FORCE)
set(SDL2MIXER_MP3 OFF CACHE BOOL "Disable MP3 support" FORCE)
set(SDL2MIXER_FLAC OFF CACHE BOOL "Disable FLAC support" FORCE)
set(SDL2MIXER_MOD OFF CACHE BOOL "Disable MOD support" FORCE)
set(SDL2MIXER_MIDI OFF CACHE BOOL "Disable MIDI support" FORCE)
set(SDL2MIXER_OPUS OFF CACHE BOOL "Disable Opus support" FORCE)
set(SDL2MIXER_WAVPACK OFF CACHE BOOL "Disable WavPack support" FORCE)
set(SDL2MIXER_GME OFF CACHE BOOL "Disable GME support" FORCE)
set(SDL2MIXER_CMD OFF CACHE BOOL "Disable command-player support" FORCE)

set(SDL2TTF_SAMPLES OFF CACHE BOOL "Disable SDL_ttf samples" FORCE)
set(SDL2TTF_VENDORED ON CACHE BOOL "Use vendored dependencies for SDL_ttf" FORCE)
set(SDL2TTF_HARFBUZZ OFF CACHE BOOL "Disable HarfBuzz for lean build" FORCE)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.11
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.4
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.1
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.22.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL2 SDL2_image SDL2_mixer SDL2_ttf)

add_executable(PratStrike
    src/AISystem.cpp
    src/AssetManager.cpp
    src/BossSystem.cpp
    src/BulletSystem.cpp
    src/Engine.cpp
    src/GameScene.cpp
    src/PhysicsSystem.cpp
    src/Quadtree.cpp
    src/RenderSystem.cpp
    src/WaveSystem.cpp
    src/main.cpp
)

target_compile_features(PratStrike PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(PratStrike PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(PratStrike PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(PratStrike
    PRIVATE
        SDL2::SDL2
        SDL2::SDL2main
        SDL2_image::SDL2_image
        SDL2_mixer::SDL2_mixer
        SDL2_ttf::SDL2_ttf
)

if(APPLE)
    find_library(CORE_AUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(CORE_GRAPHICS_FRAMEWORK CoreGraphics REQUIRED)

    target_link_libraries(PratStrike
        PRIVATE
            ${CORE_AUDIO_FRAMEWORK}
            ${AUDIO_TOOLBOX_FRAMEWORK}
            ${CORE_FOUNDATION_FRAMEWORK}
            ${CORE_GRAPHICS_FRAMEWORK}
    )
endif()

add_custom_command(
    TARGET PratStrike POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:PratStrike>/assets"
    COMMENT "Copying assets next to executable"
)

if(MSVC)
    add_custom_command(
        TARGET PratStrike POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_RUNTIME_DLLS:PratStrike>"
                "$<TARGET_FILE_DIR:PratStrike>"
        COMMAND_EXPAND_LISTS
        COMMENT "Copying SDL runtime DLLs next to executable"
    )
endif()

add_custom_target(
    run
    COMMAND "$<TARGET_FILE:PratStrike>"
    DEPENDS PratStrike
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:PratStrike>"
    USES_TERMINAL
)
