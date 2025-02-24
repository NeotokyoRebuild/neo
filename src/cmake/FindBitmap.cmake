if(OS_WINDOWS)
    set(BITMAP_LIBRARY_NAME bitmap.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(BITMAP_LIBRARY_NAME bitmap.a)
endif()

find_file(BITMAP_LIBRARY
    NAMES ${BITMAP_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(BITMAP_LIBRARY)

add_library(bitmap_bitmap STATIC IMPORTED GLOBAL)
add_library(bitmap::bitmap ALIAS bitmap_bitmap)

set_target_properties(bitmap_bitmap PROPERTIES
    IMPORTED_LOCATION "${BITMAP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/bitmap"
)
