add_executable(vrad)

target_link_libraries(vrad
    PRIVATE
    tier1::tier1
)

target_sources_grouped(
    TARGET vrad
    NAME "Source Files"
    FILES
    vrad_launcher.cpp
    StdAfx.cpp
)

target_sources_grouped(
    TARGET vrad
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/tier1/interface.h
    ${CMAKE_SOURCE_DIR}/public/ivraddll.h
    StdAfx.h
)

target_precompile_headers(vvis PUBLIC StdAfx.h)
