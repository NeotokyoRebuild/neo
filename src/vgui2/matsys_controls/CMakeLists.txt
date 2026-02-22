add_library(matsys_controls STATIC)

add_library(matsys_controls::matsys_controls ALIAS matsys_controls)

set_target_properties(matsys_controls PROPERTIES PREFIX "")

target_include_directories(matsys_controls
    PRIVATE
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/public
)

target_link_libraries(matsys_controls
    PRIVATE
    tier0::tier0
    tier1::tier1
    bitmap::bitmap
)

if (OS_LINUX)
    set_target_properties(matsys_controls PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif ()

target_sources_grouped(
    TARGET matsys_controls
    NAME "Source Files"
    FILES
    assetpicker.cpp
    baseassetpicker.cpp
    colorpickerpanel.cpp
    curveeditorpanel.cpp
    gamefiletreeview.cpp
    manipulator.cpp
    matsyscontrols.cpp
    mdlpanel.cpp
    mdlpicker.cpp
    mdlsequencepicker.cpp
    picker.cpp
    potterywheelpanel.cpp
    proceduraltexturepanel.cpp
    QCGenerator.cpp
    sequencepicker.cpp
    tgapreviewpanel.cpp
    vmtpicker.cpp
    vmtpreviewpanel.cpp
    vtfpicker.cpp
    vtfpreviewpanel.cpp
    vmtpanel.cpp
)

target_sources_grouped(
    TARGET matsys_controls
    NAME "Public Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/assetpicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/baseassetpicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/colorpickerpanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/gamefiletreeview.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/manipulator.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/matsyscontrols.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/mdlpanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/mdlpicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/mdlsequencepicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/picker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/potterywheelpanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/proceduraltexturepanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/QCGenerator.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/sequencepicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/tgapreviewpanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/vmtpicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/vmtpreviewpanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/vtfpicker.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/vtfpreviewpanel.h
    ${CMAKE_SOURCE_DIR}/public/matsys_controls/vmtpanel.h
)


