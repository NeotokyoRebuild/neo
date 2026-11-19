# neo_soundbake: bakes a map's acoustic probes into its BSP at compile time (see neo_soundbake.cpp).
#
# Built from the same acoustic code as the client (game/client/neo/audio), so the probes it places
# and the scene it bakes against are the game's own. Deployed to game/bin/<platform>, next to the
# engine's filesystem_stdio and vphysics, like VBSP and VRAD.

set(NEO_AUDIO_DIR ${CMAKE_SOURCE_DIR}/game/client/neo/audio)
set(LZMA_DIR ${CMAKE_SOURCE_DIR}/utils/lzma)

add_executable(neo_soundbake
    neo_soundbake.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
    ${CMAKE_SOURCE_DIR}/public/tier0/memoverride.cpp
    ${NEO_AUDIO_DIR}/neo_audio_geometry.cpp
    ${NEO_AUDIO_DIR}/neo_audio_geometry.h
    ${NEO_AUDIO_DIR}/neo_audio_probe_lump.cpp
    ${NEO_AUDIO_DIR}/neo_audio_probe_lump.h
    ${NEO_AUDIO_DIR}/neo_audio_probes.cpp
    ${NEO_AUDIO_DIR}/neo_audio_probes.h
    ${NEO_AUDIO_DIR}/neo_spatializer.h
    ${NEO_AUDIO_DIR}/neo_spatializer_steamaudio.cpp
    # The LZMA encoder, in the lzma_header_t format the client's CLZMA decodes.
    ${LZMA_DIR}/lzma.cpp
    ${LZMA_DIR}/C/LzFind.c
    ${LZMA_DIR}/C/LzmaDec.c
    ${LZMA_DIR}/C/LzmaEnc.c
)

target_include_directories(neo_soundbake
    PRIVATE
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/public/tier0
    ${CMAKE_SOURCE_DIR}/public/tier1
    ${CMAKE_SOURCE_DIR}/game/shared
    ${NEO_AUDIO_DIR}
    ${STEAMAUDIO_INCLUDE_DIR}
)

target_compile_definitions(neo_soundbake
    PRIVATE
    _7ZIP_ST # single-threaded LZMA match finder: no LzFindMt/Threads
    # filesystem_init.cpp names the file system library with this, as tier1 loads modules with it;
    # without it, it looks for "filesystem_stdio_DLL_EXT".
    _DLL_EXT=${SRV_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}
)

# A standalone tool: the standard library containers and std::thread the shared acoustic code
# uses need unwind semantics, and Valve's LZMA SDK sources are not warning-clean.
target_compile_options(neo_soundbake PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)
if(NOT COMPILER_MSVC)
    set_source_files_properties(
        ${LZMA_DIR}/lzma.cpp
        ${LZMA_DIR}/C/LzFind.c
        ${LZMA_DIR}/C/LzmaDec.c
        ${LZMA_DIR}/C/LzmaEnc.c
        PROPERTIES
            COMPILE_OPTIONS -w
    )
endif()
if(COMPILER_MSVC)
    set_source_files_properties(
        ${LZMA_DIR}/lzma.cpp
        ${LZMA_DIR}/C/LzFind.c
        ${LZMA_DIR}/C/LzmaDec.c
        ${LZMA_DIR}/C/LzmaEnc.c
        PROPERTIES
            COMPILE_OPTIONS /W0
    )
    # Valve's file system init binds string literals to char * (argv for vconfig), which C++20's
    # strict string rules reject; nothing else in the build compiles it.
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
        PROPERTIES
            COMPILE_OPTIONS /Zc:strictStrings-
    )
endif()

target_link_libraries(neo_soundbake
    PRIVATE
    mathlib::mathlib
    tier0::tier0
    tier1::tier1
    tier2::tier2
    vstdlib::vstdlib
)

if(OS_WINDOWS)
    target_link_libraries(neo_soundbake
        PRIVATE
        Advapi32.lib # the Steam path in the registry, for appid mounting
    )
elseif(OS_LINUX)
    target_link_libraries(neo_soundbake
        PRIVATE
        dl
        pthread # std::thread for the bake
    )
    # The engine's libtier0.so and libvstdlib.so sit beside the tool in bin/linux64; the project
    # otherwise skips rpaths (CMAKE_SKIP_RPATH), and the engine's own executables get theirs from a
    # launcher script.
    target_link_options(neo_soundbake PRIVATE "LINKER:-rpath,$ORIGIN")
endif()

target_link_directories(neo_soundbake
    PRIVATE
    ${LIBCOMMON}
    ${LIBPUBLIC}
)

add_custom_command(
    TARGET neo_soundbake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/../game/bin/${PLATSUBDIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:neo_soundbake>" "${CMAKE_SOURCE_DIR}/../game/bin/${PLATSUBDIR}/"
    VERBATIM
)
