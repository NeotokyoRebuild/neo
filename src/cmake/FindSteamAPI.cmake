if(OS_WINDOWS)
    set(STEAM_API_LIBRARY_NAME steam_api64.lib)
elseif(OS_LINUX)
    set(STEAM_API_LIBRARY_NAME libsteam_api.so)
elseif(OS_MACOS)
    set(STEAM_API_LIBRARY_NAME libsteam_api.dylib)
endif()

find_file(STEAM_API_LIBRARY
    NAMES ${STEAM_API_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(STEAM_API_LIBRARY)

add_library(steam_api_steam_api SHARED IMPORTED GLOBAL)
add_library(steam_api::steam_api ALIAS steam_api_steam_api)

if(OS_WINDOWS)
    set_target_properties(steam_api_steam_api PROPERTIES IMPORTED_IMPLIB "${STEAM_API_LIBRARY}")
elseif(OS_LINUX OR OS_MACOS)
    set_target_properties(steam_api_steam_api PROPERTIES
        IMPORTED_LOCATION "${STEAM_API_LIBRARY}"
        IMPORTED_NO_SONAME TRUE
    )
endif()

set_target_properties(steam_api_steam_api PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/steam")
