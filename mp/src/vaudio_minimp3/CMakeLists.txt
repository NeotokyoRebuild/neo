add_library(vaudio_minimp3 STATIC)

add_library(vaudio_minimp3::vaudio_minimp3 ALIAS vaudio_minimp3)

target_compile_definitions(vaudio_minimp3 PRIVATE VAUDIO_MINIMP3_STATIC_LIB)

target_include_directories(vaudio_minimp3
	PUBLIC
    ${CMAKE_SOURCE_DIR}/public/tier1
	PRIVATE
    ${CMAKE_SOURCE_DIR}/vaudio_minimp3
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/public/tier0
)

target_link_directories(vaudio_minimp3
    PRIVATE
    ${LIBCOMMON}
    ${LIBPUBLIC}
)

target_sources_grouped(
    TARGET vaudio_minimp3
    NAME "Source Files"
    FILES
    vaudio_minimp3.cpp
)

target_sources_grouped(
    TARGET vaudio_minimp3
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/tier1/interface.h
    ${CMAKE_SOURCE_DIR}/public/vaudio/ivaudio.h
    ${CMAKE_SOURCE_DIR}/vaudio_minimp3/minimp3.h
)