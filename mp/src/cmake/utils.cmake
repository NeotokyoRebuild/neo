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