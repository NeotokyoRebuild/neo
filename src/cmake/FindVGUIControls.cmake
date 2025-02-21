if(OS_WINDOWS)
    set(VGUI_CONTROLS_LIBRARY_NAME vgui_controls.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(VGUI_CONTROLS_LIBRARY_NAME vgui_controls.a)
endif()

find_file(VGUI_CONTROLS_LIBRARY
    NAMES ${VGUI_CONTROLS_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(VGUI_CONTROLS_LIBRARY)

add_library(vgui_controls_vgui_controls STATIC IMPORTED GLOBAL)
add_library(vgui_controls::vgui_controls ALIAS vgui_controls_vgui_controls)

set(VGUI_CONTROLS_INCLUDE_DIRECTORIES
    ${CMAKE_SOURCE_DIR}/public/vgui
    ${CMAKE_SOURCE_DIR}/public/vgui_controls
)

set_target_properties(vgui_controls_vgui_controls PROPERTIES
    IMPORTED_LOCATION "${VGUI_CONTROLS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${VGUI_CONTROLS_INCLUDE_DIRECTORIES}"
)

if (OS_LINUX)
    set_target_properties(vgui_controls_vgui_controls PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif ()


