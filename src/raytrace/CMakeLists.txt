add_library(raytrace OBJECT)

add_library(raytrace::raytrace ALIAS raytrace)

target_include_directories(raytrace
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/common
)

target_link_libraries(raytrace
    PRIVATE
    tier1::tier1
)

target_sources_grouped(
    TARGET raytrace
    NAME "Source Files"
    FILES
    raytrace.cpp
    trace2.cpp
    trace3.cpp
)
