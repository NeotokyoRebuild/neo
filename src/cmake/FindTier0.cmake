if(OS_WINDOWS)
    set(TIER0_LIBRARY_NAME tier0.lib)
elseif(OS_LINUX)
    set(TIER0_LIBRARY_NAME libtier0.so)
elseif(OS_MACOS)
    set(TIER0_LIBRARY_NAME libtier0.dylib)
endif()

find_file(TIER0_LIBRARY
    NAMES ${TIER0_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(TIER0_LIBRARY)

add_library(tier0_tier0 SHARED IMPORTED GLOBAL)
add_library(tier0::tier0 ALIAS tier0_tier0)

if(OS_WINDOWS)
    set_target_properties(tier0_tier0 PROPERTIES IMPORTED_IMPLIB "${TIER0_LIBRARY}")
elseif(OS_LINUX OR OS_MACOS)
    set_target_properties(tier0_tier0 PROPERTIES
        IMPORTED_LOCATION "${TIER0_LIBRARY}"
        IMPORTED_NO_SONAME TRUE
    )
endif()

set_target_properties(tier0_tier0 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/tier0")
