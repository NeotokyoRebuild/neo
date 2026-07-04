add_executable(vrad)

set_target_properties(vrad PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
)

if(NEO_USE_SEPARATE_BUILD_INFO)
    split_debug_information(TARGET vrad)
endif()

add_library_copy_target(TARGET vrad)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
    add_dependencies(vrad_copy_lib vrad_split_debug_information)
endif()

target_link_libraries(vrad
    PRIVATE
    tier1::tier1
)

target_sources_grouped(
    TARGET vrad
    NAME "Source Files"
    FILES
    vrad_launcher.cpp
    stdafx.cpp
)

target_sources_grouped(
    TARGET vrad
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/tier1/interface.h
    ${CMAKE_SOURCE_DIR}/public/ivraddll.h
    stdafx.h
)

target_precompile_headers(vrad PUBLIC stdafx.h)
