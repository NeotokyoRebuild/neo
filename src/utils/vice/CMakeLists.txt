add_executable(vice)

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
