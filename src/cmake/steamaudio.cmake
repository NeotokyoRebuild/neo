# NEO HRTF: Steam Audio SDK for the client's HRTF spatializer.
#
# Only the public headers are compiled against; the client loads the library at runtime (never
# links it), so all this does is obtain the prebuilt SDK, expose its include/ directory and copy
# its platform library next to the game libraries. The release zip is pinned by URL and SHA256:
# upgrading Steam Audio is bumping both. NEO_STEAMAUDIO_SDK_PATH points at a local SDK root
# (containing include/ and lib/, e.g. a build of ValveSoftware/steam-audio) instead.

if(NEO_STEAMAUDIO_SDK_PATH)
    set(STEAMAUDIO_SDK_DIR "${NEO_STEAMAUDIO_SDK_PATH}")
else()
    include(FetchContent)

    FetchContent_Declare(
        steamaudio
        URL https://github.com/ValveSoftware/steam-audio/releases/download/v4.8.1/steamaudio_4.8.1.zip
        URL_HASH SHA256=4a0aa5ec1176f38f0b0993a37c2259d9e86f27e22d5e24f83ec4c3cb9a1d5449
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        USES_TERMINAL_DOWNLOAD TRUE
    )

    FetchContent_MakeAvailable(steamaudio)

    set(STEAMAUDIO_SDK_DIR "${steamaudio_SOURCE_DIR}")
endif()

set(STEAMAUDIO_INCLUDE_DIR "${STEAMAUDIO_SDK_DIR}/include")

# TrueAudioNext.dll and GPUUtilities.dll ship alongside phonon.dll but are only used for
# GPU convolution, which the binaural effect does not need.
if(OS_LINUX)
    set(STEAMAUDIO_LIBRARY "${STEAMAUDIO_SDK_DIR}/lib/linux-x64/libphonon.so")
elseif(OS_WINDOWS)
    set(STEAMAUDIO_LIBRARY "${STEAMAUDIO_SDK_DIR}/lib/windows-x64/phonon.dll")
else()
    message(FATAL_ERROR "NEO_STEAMAUDIO is only supported on Linux and Windows")
endif()

if(NOT EXISTS "${STEAMAUDIO_LIBRARY}")
    message(FATAL_ERROR "Steam Audio library not found: ${STEAMAUDIO_LIBRARY}")
endif()

message(STATUS "Steam Audio library: ${STEAMAUDIO_LIBRARY}")

add_custom_target(
    steamaudio_copy_lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${STEAMAUDIO_LIBRARY}" "${NEO_OUTPUT_LIBRARY_PATH}/"
    VERBATIM
)
