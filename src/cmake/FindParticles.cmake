if(OS_WINDOWS)
    set(PARTICLES_LIBRARY_NAME particles.lib)
elseif(OS_LINUX OR OS_MACOS)
    set(PARTICLES_LIBRARY_NAME particles.a)
endif()

find_file(PARTICLES_LIBRARY
    NAMES ${PARTICLES_LIBRARY_NAME}
    PATHS "${LIBPUBLIC}"
    NO_CACHE
    NO_DEFAULT_PATH
    REQUIRED
)

mark_as_advanced(PARTICLES_LIBRARY)

add_library(particles_particles STATIC IMPORTED GLOBAL)
add_library(particles::particles ALIAS particles_particles)

set_target_properties(particles_particles PROPERTIES
    IMPORTED_LOCATION "${PARTICLES_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/public/particles"
)
