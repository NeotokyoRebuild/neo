if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPU_64BIT TRUE)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CPU_32BIT TRUE)
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

    add_custom_target(
        ${PARSED_ARGS_TARGET}_copy_lib
        DEPENDS ${PARSED_ARGS_TARGET} ${PARSED_ARGS_TARGET}_copy_lib_command
    )

    add_custom_command(
        OUTPUT ${PARSED_ARGS_TARGET}_copy_lib_command
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PARSED_ARGS_TARGET}> "${NEO_OUTPUT_LIBRARY_PATH}/"
        WORKING_DIRECTORY "${NEO_OUTPUT_LIBRARY_PATH}"
        VERBATIM
    )
endfunction()

# Used by split_debug_information
if(NOT MSVC)
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

    if(MSVC)
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

    # NOTE: Prefix expression was simplified - it probably doesn't work for static libraries
    get_property(TARGET_PREFIX TARGET ${PARSED_ARGS_TARGET} PROPERTY PREFIX)
    get_property(TARGET_POSTFIX TARGET ${PARSED_ARGS_TARGET} PROPERTY POSTFIX)
    get_property(TARGET_OUTPUT_NAME TARGET ${PARSED_ARGS_TARGET} PROPERTY OUTPUT_NAME)

    if("${TARGET_OUTPUT_NAME}" STREQUAL "")
        set(TARGET_OUTPUT_NAME "${PARSED_ARGS_TARGET}")
    endif()

    set(OUTPUT_NAME_FULL "${TARGET_PREFIX}${TARGET_OUTPUT_NAME}${TARGET_POSTFIX}.debug")

    set(SPLITDEBUG_SOURCE "$<TARGET_FILE:${PARSED_ARGS_TARGET}>")
    set(SPLITDEBUG_TARGET "$<TARGET_FILE_DIR:${PARSED_ARGS_TARGET}>/${OUTPUT_NAME_FULL}")

    # NOTE: objcopy --strip-debug does NOT fully strip the binary; two sections are left:
    # - .symtab: Symbol table.
    # - .strtab: String table.
    # These sections are split into the .debug file, so there's no reason to keep them in the executable
    add_custom_command(TARGET ${PARSED_ARGS_TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --only-keep-debug ${OBJCOPY_COMPRESS_DEBUG_SECTIONS_PARAM} ${SPLITDEBUG_SOURCE} ${SPLITDEBUG_TARGET}
        COMMAND ${CMAKE_STRIP} ${SPLITDEBUG_SOURCE}
        COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="${SPLITDEBUG_TARGET}" ${SPLITDEBUG_SOURCE}
    )

    # Set the target property to allow installation
    set_target_properties(${PARSED_ARGS_TARGET} PROPERTIES
        SPLIT_DEBUG_INFO_FILE ${SPLITDEBUG_TARGET}
    )

    # Make sure the file is deleted on `make clean`
    set_property(DIRECTORY APPEND
        PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${SPLITDEBUG_TARGET}
    )
endfunction()
