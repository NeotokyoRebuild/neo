if(OS_WINDOWS)
    set(CHOREO_OBJECTS_LIBRARY_NAME choreoobjects.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(CHOREO_OBJECTS_LIBRARY_NAME choreoobjects.a)
endif()

find_file(CHOREO_OBJECTS_LIBRARY
    NAMES ${CHOREO_OBJECTS_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(CHOREO_OBJECTS_LIBRARY)

add_library(choreo_objects_choreo_objects STATIC IMPORTED GLOBAL)
add_library(choreo_objects::choreo_objects ALIAS choreo_objects_choreo_objects)

set_target_properties(choreo_objects_choreo_objects PROPERTIES
    IMPORTED_LOCATION "${CHOREO_OBJECTS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/game/shared"
)
