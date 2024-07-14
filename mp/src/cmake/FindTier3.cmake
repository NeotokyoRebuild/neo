if(OS_WINDOWS)
    set(TIER3_LIBRARY_NAME tier3.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(TIER3_LIBRARY_NAME tier3.a)
endif()

find_library(TIER3_LIBRARY
    NAMES ${TIER3_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(TIER3_LIBRARY)

add_library(tier3_tier3 STATIC IMPORTED GLOBAL)
add_library(tier3::tier3 ALIAS tier3_tier3)

set_target_properties(tier3_tier3 PROPERTIES
    IMPORTED_LOCATION "${TIER3_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/tier3"
)
