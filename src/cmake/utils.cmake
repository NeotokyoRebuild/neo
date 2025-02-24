if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPU_64BIT TRUE)
    message(STATUS "64-bit build")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CPU_32BIT TRUE)
    message(STATUS "32-bit build")
endif()

if(WIN32)
    set(OS_WINDOWS TRUE)
elseif(UNIX)
    if(APPLE)
        set(OS_MACOS TRUE)
    else()
        set(OS_LINUX TRUE)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COMPILER_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMPILER_GCC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(COMPILER_MSVC TRUE)
endif()

function(target_sources_grouped)
    cmake_parse_arguments(
        PARSED_ARGS
        ""
        "TARGET;NAME;SCOPE"
        "FILES"
        ${ARGN}
    )

    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target name")
    endif()

    if(NOT PARSED_ARGS_NAME)
        message(FATAL_ERROR "You must provide a source group name")
    endif()

    if(NOT PARSED_ARGS_SCOPE)
        set(PARSED_ARGS_SCOPE PRIVATE)
    endif()

    target_sources(${PARSED_ARGS_TARGET} ${PARSED_ARGS_SCOPE} ${PARSED_ARGS_FILES})

    source_group(${PARSED_ARGS_NAME} FILES ${PARSED_ARGS_FILES})
endfunction()

function(add_library_copy_target)
    cmake_parse_arguments(
        PARSED_ARGS
        ""
        "TARGET"
        ""
        ${ARGN}
    )

    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target name")
    endif()

    get_target_property(TARGET_STRIPPED_FILE ${PARSED_ARGS_TARGET} STRIPPED_FILE)
    if(NOT TARGET_STRIPPED_FILE)
        set(TARGET_STRIPPED_FILE $<TARGET_FILE:${PARSED_ARGS_TARGET}>)
    endif()

    add_custom_target(
        ${PARSED_ARGS_TARGET}_copy_lib
        DEPENDS ${PARSED_ARGS_TARGET} ${PARSED_ARGS_TARGET}_copy_lib_command
    )

    add_custom_command(
        OUTPUT ${PARSED_ARGS_TARGET}_copy_lib_command
        COMMAND ${CMAKE_COMMAND} -E copy ${TARGET_STRIPPED_FILE} "${NEO_OUTPUT_LIBRARY_PATH}/"
        WORKING_DIRECTORY "${NEO_OUTPUT_LIBRARY_PATH}"
        VERBATIM
    )

endfunction()

# Used by split_debug_information
if(NOT COMPILER_MSVC)
    include(CMakeFindBinUtils)
endif()

function(split_debug_information)
    cmake_parse_arguments(
        PARSED_ARGS
        ""
        "TARGET"
        ""
        ${ARGN}
    )

    if(COMPILER_MSVC)
        # MSVC splits debug information by itself
        return()
    elseif(NOT CMAKE_OBJCOPY)
        message(FATAL_ERROR "'objcopy' is not found")
    elseif(NOT CMAKE_STRIP)
        message(FATAL_ERROR "'strip' is not found")
    endif()

    # If the linker doesn't support --compress-debug-sections=zlib, check if objcopy supports --compress-debug-sections
    if(LDFLAG_--compress-debug-sections)
        # ld supports --compress-debug-sections=zlib.
        set(OBJCOPY_COMPRESS_DEBUG_SECTIONS_PARAM "" CACHE INTERNAL "objcopy parameter to compress debug sections")
    elseif(NOT LDFLAG_--compress-debug-sections AND NOT DEFINED OBJCOPY_COMPRESS_DEBUG_SECTIONS_PARAM)
        # Check for objcopy --compress-debug-sections.
        message(STATUS "Checking if objcopy supports --compress-debug-sections")
        execute_process(COMMAND ${CMAKE_OBJCOPY} --help
            OUTPUT_VARIABLE xc_out
            ERROR_QUIET
        )
        if(xc_out MATCHES "--compress-debug-sections")
            # objcopy has --compress-debug-sections
            message(STATUS "Checking if objcopy supports --compress-debug-sections - yes")
            set(OBJCOPY_COMPRESS_DEBUG_SECTIONS_PARAM "--compress-debug-sections" CACHE INTERNAL "objcopy parameter to compress debug sections")
        else()
            # objcopy does *not* have --compress-debug-sections
            message(STATUS "Checking if objcopy supports --compress-debug-sections - no")
            set(OBJCOPY_COMPRESS_DEBUG_SECTIONS_PARAM "" CACHE INTERNAL "objcopy parameter to compress debug sections")
        endif()
        unset(xc_out)
    endif()

    get_target_property(TARGET_OUTPUT_NAME ${PARSED_ARGS_TARGET} OUTPUT_NAME)

    if(NOT TARGET_OUTPUT_NAME)
        set(TARGET_OUTPUT_NAME "${PARSED_ARGS_TARGET}")
    endif()

    set(SPLITDEBUG_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_OUTPUT_NAME}.so")
    set(SPLITDEBUG_TARGET "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SUBDIRECTORY}/${TARGET_OUTPUT_NAME}.so")
    set(SPLITDEBUG_TARGET_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SUBDIRECTORY}/${TARGET_OUTPUT_NAME}.so.debug")

    add_custom_target(
        ${PARSED_ARGS_TARGET}_split_debug_information
        ALL
        DEPENDS ${PARSED_ARGS_TARGET} ${SPLITDEBUG_TARGET}
    )

    # NOTE: objcopy --strip-debug does NOT fully strip the binary; two sections are left:
    # - .symtab: Symbol table.
    # - .strtab: String table.
    # These sections are split into the .debug file, so there's no reason to keep them in the executable
    add_custom_command(
        OUTPUT ${SPLITDEBUG_TARGET}
        COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${OBJCOPY_COMPRESS_DEBUG_SECTIONS_PARAM} ${SPLITDEBUG_SOURCE} ${SPLITDEBUG_TARGET_DEBUG}
        COMMAND ${CMAKE_STRIP} ${SPLITDEBUG_SOURCE} -o ${SPLITDEBUG_TARGET}
        COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="${SPLITDEBUG_TARGET_DEBUG}" ${SPLITDEBUG_TARGET}
    )

    # Set the target property to allow installation
    set_target_properties(${PARSED_ARGS_TARGET} PROPERTIES
        STRIPPED_FILE ${SPLITDEBUG_TARGET}
        SPLIT_DEBUG_INFO_FILE ${SPLITDEBUG_TARGET_DEBUG}
    )

    # Make sure the file is deleted when cleaning
    set_target_properties(${PARSED_ARGS_TARGET}
        PROPERTIES
        ADDITIONAL_CLEAN_FILES "${SPLITDEBUG_TARGET};${SPLITDEBUG_TARGET_DEBUG}"
    )
endfunction()

function(add_gamedata_gen_target)
    cmake_parse_arguments(
        PARSED_ARGS
        ""
        "TARGET"
        ""
        ${ARGN}
    )

    if(NOT PARSED_ARGS_TARGET)
        message(FATAL_ERROR "You must provide a target name")
    endif()

    set(GAMEDATA_LIBRARY "$<TARGET_FILE:${PARSED_ARGS_TARGET}>")

    set(GAMEDATA_INPUT_FILES
        "${CMAKE_SOURCE_DIR}/gamedata/sdkhooks.games/game.neo.txt.in"
        "${CMAKE_SOURCE_DIR}/gamedata/sdktools.games/game.neo.txt.in"
    )

    set(GAMEDATA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SUBDIRECTORY})
    set(GAMEDATA_SDKHOOKS_OUTPUT_FILE "${GAMEDATA_OUTPUT_DIR}/sdkhooks.games/game.neo.txt")
    set(GAMEDATA_SDKTOOLS_OUTPUT_FILE "${GAMEDATA_OUTPUT_DIR}/sdktools.games/game.neo.txt")

    set(GAMEDATA_OUTPUT_FILES
        "${GAMEDATA_SDKHOOKS_OUTPUT_FILE}"
        "${GAMEDATA_SDKTOOLS_OUTPUT_FILE}"
    )

    set(GAMEDATA_OUTPUT_DIRS
        "${GAMEDATA_OUTPUT_DIR}/sdkhooks.games"
        "${GAMEDATA_OUTPUT_DIR}/sdktools.games"
    )

    add_custom_target(
        ${PARSED_ARGS_TARGET}_generate_gamedata
        ALL
        DEPENDS ${PARSED_ARGS_TARGET} ${GAMEDATA_OUTPUT_FILES}
    )

    add_custom_command(
        OUTPUT ${GAMEDATA_OUTPUT_FILES}
        COMMAND ${GAMEDATA_GEN_PATH}
            --library ${GAMEDATA_LIBRARY}
            --input_files ${GAMEDATA_INPUT_FILES}
            --output_dirs ${GAMEDATA_OUTPUT_DIRS}
        DEPENDS ${GAMEDATA_INPUT_FILES}
        VERBATIM
    )

    set_target_properties(${PARSED_ARGS_TARGET} PROPERTIES
        GAMEDATA_SDKHOOKS_OUTPUT_FILE "${GAMEDATA_SDKHOOKS_OUTPUT_FILE}"
        GAMEDATA_SDKTOOLS_OUTPUT_FILE "${GAMEDATA_SDKTOOLS_OUTPUT_FILE}"
        ADDITIONAL_CLEAN_FILES "${GAMEDATA_OUTPUT_FILES}"
    )
endfunction()
