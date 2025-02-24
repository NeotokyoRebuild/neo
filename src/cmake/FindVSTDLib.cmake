if(OS_WINDOWS)
    set(VSTDLIB_LIBRARY_NAME vstdlib.lib)
elseif(OS_LINUX)
    set(VSTDLIB_LIBRARY_NAME libvstdlib.so)
elseif(OS_MACOS)
    set(VSTDLIB_LIBRARY_NAME libvstdlib.dylib)
endif()

find_library(VSTDLIB_LIBRARY
    NAMES ${VSTDLIB_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(VSTDLIB_LIBRARY)

add_library(vstdlib_vstdlib SHARED IMPORTED GLOBAL)
add_library(vstdlib::vstdlib ALIAS vstdlib_vstdlib)

if(OS_WINDOWS)
    set_target_properties(vstdlib_vstdlib PROPERTIES IMPORTED_IMPLIB "${VSTDLIB_LIBRARY}")
elseif(OS_LINUX OR OS_MACOS)
    set_target_properties(vstdlib_vstdlib PROPERTIES
        IMPORTED_LOCATION "${VSTDLIB_LIBRARY}"
        IMPORTED_NO_SONAME TRUE
    )
endif()

set_target_properties(vstdlib_vstdlib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/vstdlib")
