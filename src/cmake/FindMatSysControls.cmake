if(OS_WINDOWS)
    set(MATSYS_CONTROLS_LIBRARY_NAME matsys_controls.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(MATSYS_CONTROLS_LIBRARY_NAME matsys_controls.a)
endif()

find_file(MATSYS_CONTROLS_LIBRARY
    NAMES ${MATSYS_CONTROLS_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(MATSYS_CONTROLS_LIBRARY)

add_library(matsys_controls_matsys_controls STATIC IMPORTED GLOBAL)
add_library(matsys_controls::matsys_controls ALIAS matsys_controls_matsys_controls)

set_target_properties(matsys_controls_matsys_controls PROPERTIES
    IMPORTED_LOCATION "${MATSYS_CONTROLS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/matsys_controls"
)
