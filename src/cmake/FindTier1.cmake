if(OS_WINDOWS)
    set(TIER1_LIBRARY_NAME tier1.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(TIER1_LIBRARY_NAME tier1.a)
endif()

find_library(TIER1_LIBRARY
    NAMES ${TIER1_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(TIER1_LIBRARY)

add_library(tier1_tier1 STATIC IMPORTED GLOBAL)
add_library(tier1::tier1 ALIAS tier1_tier1)

set_target_properties(tier1_tier1 PROPERTIES
    IMPORTED_LOCATION "${TIER1_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/tier1"
)

if (OS_LINUX)
    set_target_properties(tier1_tier1 PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif ()

