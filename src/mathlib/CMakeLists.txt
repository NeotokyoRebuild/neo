add_library(mathlib STATIC)

add_library(mathlib::mathlib ALIAS mathlib)

set_target_properties(mathlib PROPERTIES
    PREFIX ""
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
)

target_include_directories(mathlib
    PRIVATE
    ${CMAKE_SOURCE_DIR}/public/mathlib
)

target_compile_definitions(mathlib
    PRIVATE
    MATHLIB_LIB
)

target_link_libraries(mathlib
    PRIVATE
    tier1::tier1
)

if(OS_WINDOWS)
    target_compile_definitions(mathlib
        PRIVATE
        fopen=dont_use_fopen
    )
elseif(OS_LINUX)
    set_target_properties(mathlib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

set(UNITY_SOURCE_MATHLIB
    color_conversion.cpp
    halton.cpp
    lightdesc.cpp
    mathlib_base.cpp
    powsse.cpp
    sparse_convolution_noise.cpp
    sseconst.cpp
    sse.cpp
    ssenoise.cpp				
    anorms.cpp
    bumpvects.cpp
    IceKey.cpp
    polyhedron.cpp
    randsse.cpp
    spherical.cpp
    simdvectormatrix.cpp
    vmatrix.cpp
    almostequal.cpp
)

set_source_files_properties(
    ${UNITY_SOURCE_MATHLIB}
    PROPERTIES UNITY_GROUP mathlib
)

target_sources_grouped(
    TARGET mathlib
    NAME "Source Files"
    FILES
    ${UNITY_SOURCE_MATHLIB}
    imagequant.cpp
    quantize.cpp
	#vector.cpp # Unused
)

if(OS_LINUX OR OS_MACOS)
    set_source_files_properties(
        sseconst.cpp
        sse.cpp
        PROPERTIES
        COMPILE_FLAGS -Wno-narrowing
    )
endif()

if(OS_WINDOWS OR OS_LINUX)
    target_sources_grouped(
	TARGET mathlib
	NAME "Source Files"
	FILES
	3dnow.cpp
    )
endif()

target_sources_grouped(
    TARGET mathlib
    NAME "Public Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/mathlib/anorms.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/bumpvects.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/compressed_3d_unitvec.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/compressed_light_cube.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/compressed_vector.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/halton.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/IceKey.H
    ${CMAKE_SOURCE_DIR}/public/mathlib/lightdesc.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/math_pfns.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/mathlib.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/noise.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/polyhedron.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/quantize.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/simdvectormatrix.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/spherical_geometry.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/ssemath.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/ssequaternion.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector2d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector4d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vmatrix.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vplane.h
)

if(OS_WINDOWS OR OS_LINUX)
    target_sources_grouped(
	TARGET mathlib
	NAME "Public Header Files"
	FILES
	${CMAKE_SOURCE_DIR}/public/mathlib/amd3dx.h
    )
endif()

target_sources_grouped(
    TARGET mathlib
    NAME "Header Files"
    FILES
    noisedata.h
    sse.h
)

if(OS_WINDOWS OR OS_LINUX)
    target_sources_grouped(
	TARGET mathlib
	NAME "Header Files"
	FILES
	3dnow.h
    )
endif()
