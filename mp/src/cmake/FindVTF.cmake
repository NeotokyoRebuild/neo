if(OS_WINDOWS)
    set(VTF_LIBRARY_NAME vtf.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(VTF_LIBRARY_NAME vtf.a)
endif()

find_file(VTF_LIBRARY
    NAMES ${VTF_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(VTF_LIBRARY)

add_library(vtf_vtf STATIC IMPORTED GLOBAL)
add_library(vtf::vtf ALIAS vtf_vtf)

set_target_properties(vtf_vtf PROPERTIES
    IMPORTED_LOCATION "${VTF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/vtf"
)
