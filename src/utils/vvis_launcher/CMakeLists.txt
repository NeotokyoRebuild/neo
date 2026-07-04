add_executable(vvis)

set_target_properties(vvis PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
)

if(NEO_USE_SEPARATE_BUILD_INFO)
    split_debug_information(TARGET vvis)
endif()

add_library_copy_target(TARGET vvis)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
    add_dependencies(vvis_copy_lib vvis_split_debug_information)
endif()

target_link_libraries(vvis
    PRIVATE
    tier1::tier1
)

target_sources_grouped(
    TARGET vvis
    NAME "Source Files"
    FILES
    vvis_launcher.cpp
    StdAfx.cpp
)

target_sources_grouped(
    TARGET vvis
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/tier1/interface.h
    StdAfx.h
)

target_precompile_headers(vvis PUBLIC StdAfx.h)
