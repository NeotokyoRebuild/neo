add_executable(vice)

set_target_properties(vice PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
)

if(NEO_USE_SEPARATE_BUILD_INFO)
    split_debug_information(TARGET vice)
endif()

add_library_copy_target(TARGET vice)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
    add_dependencies(vice_copy_lib vice_split_debug_information)
endif()

target_include_directories(vice
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/common
)

target_link_libraries(vice
    PRIVATE
    mathlib::mathlib
    tier2::tier2
    vstdlib::vstdlib
    Threads::Threads
)

target_sources_grouped(
    TARGET vice
    NAME "Source Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
    ../common/cmdlib.cpp
    ../common/filesystem_tools.cpp
    vice.cpp
)

target_sources_grouped(
    TARGET vice
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/mathlib/IceKey.H
)
