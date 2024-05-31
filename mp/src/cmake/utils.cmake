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

function(add_library_copy_and_symlink)
    cmake_parse_arguments(
        PARSED_ARGS
        "CREATE_SRV"
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

    if(OS_LINUX AND PARSED_ARGS_CREATE_SRV)
        add_custom_command(
            OUTPUT ${PARSED_ARGS_TARGET}_copy_lib_command APPEND
            COMMAND ln -sf ${PARSED_ARGS_TARGET}.so ${PARSED_ARGS_TARGET}_srv.so
            VERBATIM
        )
    endif()

    set_source_files_properties(${PARSED_ARGS_TARGET}_copy_lib_command PROPERTIES SYMBOLIC "true")
endfunction()
