if(OS_WINDOWS)
    set(DMX_LOADER_LIBRARY_NAME dmxloader.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(DMX_LOADER_LIBRARY_NAME dmxloader.a)
endif()

find_file(DMX_LOADER_LIBRARY
    NAMES ${DMX_LOADER_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(DMX_LOADER_LIBRARY)

add_library(dmx_loader_dmx_loader STATIC IMPORTED GLOBAL)
add_library(dmx_loader::dmx_loader ALIAS dmx_loader_dmx_loader)

set_target_properties(dmx_loader_dmx_loader PROPERTIES
    IMPORTED_LOCATION "${DMX_LOADER_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/dmxloader"
)
