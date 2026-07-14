# Steam mod launcher (Linux). Valve's prebuilt Linux launcher never sets
# SDK_EXEC_DIR before exec'ing hl2.sh, so relative SDK search paths in
# gameinfo.txt silently fail to mount on standalone (non-sourcemods) installs.
# So we build it ourselves.
find_package(SDL2 REQUIRED)

add_executable(launcher_main
    main.cpp
)

# GetExecutableModName derives the default -game dir from the executable name
set_target_properties(launcher_main
    PROPERTIES
        OUTPUT_NAME neo
)

target_include_directories(launcher_main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/public
)

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

if(NEO_MOD_APPID)
    target_compile_definitions(launcher_main
        PRIVATE
            MOD_LAUNCHER
            MOD_APPID=${NEO_MOD_APPID}
    )
endif()

if(NEO_COPY_LIBRARIES)
    add_custom_command(TARGET launcher_main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:launcher_main>
            ${CMAKE_SOURCE_DIR}/../game/
        VERBATIM
    )
endif()
