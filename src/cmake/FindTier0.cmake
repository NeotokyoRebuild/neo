if(OS_WINDOWS)
    set(TIER0_LIBRARY_NAME tier0.lib)
elseif(OS_LINUX)   
    if (NEO_DEDICATED)
        set(TIER0_LIBRARY_FIND_PATH "${CMAKE_BINARY_DIR}/${LIBPUBLIC_RELATIVE_PATH}")
        set(TIER0_LIBRARY_NAME libtier0_srv.so)
        
        file(MAKE_DIRECTORY "${TIER0_LIBRARY_FIND_PATH}")
        file(COPY_FILE "${LIBPUBLIC}/libtier0.so" "${TIER0_LIBRARY_FIND_PATH}/${TIER0_LIBRARY_NAME}")
    else()
        set(TIER0_LIBRARY_FIND_PATH "${LIBPUBLIC}")
        set(TIER0_LIBRARY_NAME libtier0.so)
    endif()
elseif(OS_MACOS)
    set(TIER0_LIBRARY_NAME libtier0.dylib)
endif()

find_file(TIER0_LIBRARY
    NAMES ${TIER0_LIBRARY_NAME}
    PATHS "${TIER0_LIBRARY_FIND_PATH}"
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
