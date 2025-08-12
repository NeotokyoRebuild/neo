add_library(fgdlib STATIC)

add_library(fgdlib::fgdlib ALIAS fgdlib)

set_target_properties(fgdlib PROPERTIES PREFIX "")

target_include_directories(fgdlib
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils/common
)

if(OS_WINDOWS)
    target_compile_options(fgdlib
        PRIVATE
        /wd4091
        /wd4316
        /wd4355
        /wd4577
        /wd5033
        /wd5054
        /wd5055
    )
elseif(OS_LINUX)

endif()

target_link_libraries(fgdlib
    PRIVATE
    tier1::tier1
    mathlib::mathlib
)

target_sources_grouped(
    TARGET fgdlib
    NAME "Source Files"
    FILES
    gamedata.cpp
    gdclass.cpp
    gdvar.cpp
    inputoutput.cpp
    wckeyvalues.cpp
)

target_sources_grouped(
    TARGET fgdlib
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/fgdlib/fgdlib.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/gamedata.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/gdclass.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/gdvar.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/helperinfo.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/ieditortexture.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/inputoutput.h
    ${CMAKE_SOURCE_DIR}/public/fgdlib/wckeyvalues.h
)
