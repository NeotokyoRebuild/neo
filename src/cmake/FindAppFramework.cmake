if(OS_WINDOWS)
    set(APPFRAMEWORK_LIBRARY_NAME appframework.lib)
elseif(OS_LINUX)
    set(APPFRAMEWORK_LIBRARY_NAME appframework.a)
endif()

find_file(APPFRAMEWORK_LIBRARY
    NAMES ${APPFRAMEWORK_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(APPFRAMEWORK_LIBRARY)

add_library(appframework_appframework STATIC IMPORTED GLOBAL)
add_library(appframework::appframework ALIAS appframework_appframework)

set_target_properties(appframework_appframework PROPERTIES
    IMPORTED_LOCATION "${APPFRAMEWORK_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/appframework"
)
