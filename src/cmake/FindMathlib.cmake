find_library(MATHLIB_LIBRARY
    NAMES mathlib.a
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(MATHLIB_LIBRARY)

add_library(mathlib_mathlib STATIC IMPORTED GLOBAL)
add_library(mathlib::mathlib ALIAS mathlib_mathlib)

set_target_properties(mathlib_mathlib PROPERTIES
    IMPORTED_LOCATION "${MATHLIB_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/mathlib"
)
