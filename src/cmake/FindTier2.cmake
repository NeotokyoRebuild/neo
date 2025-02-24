if(OS_WINDOWS)
    set(TIER2_LIBRARY_NAME tier2.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(TIER2_LIBRARY_NAME tier2.a)
endif()

find_library(TIER2_LIBRARY
    NAMES ${TIER2_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(TIER2_LIBRARY)

add_library(tier2_tier2 STATIC IMPORTED GLOBAL)
add_library(tier2::tier2 ALIAS tier2_tier2)

set_target_properties(tier2_tier2 PROPERTIES
    IMPORTED_LOCATION "${TIER2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/tier2"
)
