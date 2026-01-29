add_executable(vvis)

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
