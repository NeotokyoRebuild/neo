add_library(lzma STATIC)

add_library(lzma::lzma ALIAS lzma)

set_target_properties(lzma PROPERTIES PREFIX "")

target_include_directories(lzma
    PRIVATE
    ${CMAKE_SOURCE_DIR}/public
    C
)

target_compile_definitions(lzma
    PRIVATE
    _NO_EXCEPTIONS
    _LZMA_PROB32
    _7ZIP_ST
)

target_link_libraries(lzma
    PRIVATE
    tier1::tier1
    mathlib::mathlib
)

target_sources_grouped(
    TARGET lzma
    NAME "Source Files"
    FILES
    lzma.cpp
)

target_sources_grouped(
    TARGET lzma
    NAME "7Zip Common"
    FILES
    C/7zTypes.h
    C/LzmaEnc.h
    C/LzmaEnc.c
    C/LzmaDec.c
    C/LzmaDec.h
    C/LzFind.h
    C/LzFind.c
    C/LzHash.h
)
