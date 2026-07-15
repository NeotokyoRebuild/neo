# Steam mod launcher (Linux). Valve's prebuilt Linux launcher never sets
# SDK_EXEC_DIR before exec'ing hl2.sh, so relative SDK search paths in
# gameinfo.txt silently fail to mount on standalone (non-sourcemods) installs.
# So we build it ourselves.
if(OS_LINUX)
find_package(SDL2 REQUIRED)
endif()

set(LAUNCHER_SOURCES main.cpp)

if(OS_WINDOWS)
    list(APPEND LAUNCHER_SOURCES launcher_main_mod_neo.rc)
endif()

add_executable(launcher_main
    ${LAUNCHER_SOURCES}
)

# GetExecutableModName derives the default -game dir from the executable name
# by stripping from the last underscore: neo_linux64 -> -game neo. The suffix
# also avoids colliding with the game/neo directory at the install root.
if(OS_LINUX)
set_target_properties(launcher_main
    PROPERTIES
        OUTPUT_NAME neo_linux64
)
elseif(OS_WINDOWS)
set_target_properties(launcher_main
    PROPERTIES
        OUTPUT_NAME neo_win64
        WIN32_EXECUTABLE TRUE
)
endif()

target_include_directories(launcher_main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/public
        ${CMAKE_SOURCE_DIR}/common
)

if(OS_LINUX)
target_link_libraries(launcher_main
    PRIVATE
        SDL2::SDL2
        ${CMAKE_DL_LIBS}
)

# main.cpp defines __wrap_fopen/__wrap_fopen64 which reference __real_fopen/__real_fopen64
target_link_options(launcher_main
    PRIVATE
        -Wl,--wrap=fopen
        -Wl,--wrap=fopen64
)
endif()

if(NEO_MOD_APPID)
    target_compile_definitions(launcher_main
        PRIVATE
            MOD_LAUNCHER
            MOD_APPID=${NEO_MOD_APPID}
    )
endif()

if(NEO_COPY_LIBRARIES)
    # Stage the standalone-install layout: launcher at the install root next to
    # neo/, and the steam_api lib where the launcher dlopens it.
    add_custom_command(TARGET launcher_main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:launcher_main>
            ${CMAKE_SOURCE_DIR}/../game/
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/../game/bin/linux64
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/lib/public/linux64/libsteam_api.so
            ${CMAKE_SOURCE_DIR}/../game/bin/linux64/
        VERBATIM
    )
endif()
