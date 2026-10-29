if(OS_WINDOWS)
    set(LIBZ_LIBRARY_NAME libz.lib)
elseif(OS_LINUX OR OS_MACOS)
    return() # TODO Linux
endif()

find_file(LIBZ_LIBRARY
    NAMES ${LIBZ_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(LIBZ_LIBRARY)

add_library(libz_libz STATIC IMPORTED GLOBAL)
add_library(libz::libz ALIAS libz_libz)

set_target_properties(libz_libz PROPERTIES
    IMPORTED_LOCATION "${LIBZ_LIBRARY}"
)
