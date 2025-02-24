find_package(Appframework)
find_package(Bitmap)
find_package(ChoreoObjects)
find_package(DMXLoader)
find_package(MatSysControls)

if(NEO_BUILD_MATHLIB)
    add_subdirectory(../mathlib ${CMAKE_BINARY_DIR}/mathlib)
else()
    find_package(Mathlib)
endif()

find_package(Particles)
find_package(SteamAPI)
find_package(Tier0)

if(NEO_BUILD_TIER1)
    add_subdirectory(../tier1 ${CMAKE_BINARY_DIR}/tier1)
else()
    find_package(Tier1)
endif()

find_package(Tier2)
find_package(Tier3)

if(NEO_BUILD_VGUI_CONTROLS)
    add_subdirectory(../vgui2/vgui_controls ${CMAKE_BINARY_DIR}/vgui2/vgui_controls)
else()
    find_package(VGUIControls)
endif()

find_package(VSTDLib)
find_package(VTF)
