add_library(vvis_library SHARED)

set_target_properties(vvis_library PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
)

if(OS_LINUX)
    set_target_properties(vvis_library PROPERTIES
        OUTPUT_NAME "vvis"
    )
endif()

if(NEO_USE_SEPARATE_BUILD_INFO)
    split_debug_information(TARGET vvis_library)
endif()

add_library_copy_target(TARGET vvis_library)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
    add_dependencies(vvis_library_copy_lib vvis_library_split_debug_information)
endif()

target_include_directories(vvis_library
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/common
)

target_compile_definitions(vvis_library
    PRIVATE
    PROTECTED_THINGS_DISABLE
)

if(OS_WINDOWS)
    target_compile_options(vvis_library
        PRIVATE
        /wd4091
        /wd4316
        /wd4355
        /wd4577
        /wd5033
        /wd5054
        /wd5055
    )

    target_link_libraries(vvis_library
        PRIVATE
        odbc32.lib
        odbccp32.lib
        ws2_32.lib
    )
endif()

set_source_files_properties(${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp PROPERTIES
    COMPILE_OPTIONS "-D_DLL_EXT=${SRV_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX};-D_EXTERNAL_DLL_EXT=${SRV_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

target_link_libraries(vvis_library
    PRIVATE
    vstdlib::vstdlib
    mathlib::mathlib
    tier2::tier2
    lzma::lzma
    Threads::Threads
)

target_sources_grouped(
    TARGET vvis_library
    NAME "Source Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/collisionutils.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
    ${CMAKE_SOURCE_DIR}/public/loadcmdline.cpp
    ${CMAKE_SOURCE_DIR}/public/lumpfiles.cpp
    ${CMAKE_SOURCE_DIR}/public/scratchpad3d.cpp
    #${CMAKE_SOURCE_DIR}/public/tier0/memoverride.cpp"
    ${CMAKE_SOURCE_DIR}/public/zip_utils.cpp
    #../common/MySqlDatabase.cpp
    ../common/bsplib.cpp
    ../common/cmdlib.cpp
    ../common/filesystem_tools.cpp
    ../common/pacifier.cpp
    ../common/scratchpad_helpers.cpp
    ../common/scriplib.cpp
    ../common/threads.cpp
    ../common/tools_minidump.cpp
    ../common/tools_minidump.h
    WaterDist.cpp
    flow.cpp
    vvis.cpp
)

target_sources_grouped(
    TARGET vvis_library
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/bspfile.h
    ${CMAKE_SOURCE_DIR}/public/bsptreedata.h
    ${CMAKE_SOURCE_DIR}/public/gamebspfile.h
    ${CMAKE_SOURCE_DIR}/public/bspflags.h
    ${CMAKE_SOURCE_DIR}/public/cmodel.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/amd3dx.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/bumpvects.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/mathlib.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector2d.h
    ${CMAKE_SOURCE_DIR}/public/tier0/basetypes.h
    ${CMAKE_SOURCE_DIR}/public/tier0/commonmacros.h
    ${CMAKE_SOURCE_DIR}/public/tier1/byteswap.h
    ${CMAKE_SOURCE_DIR}/public/tier1/checksum_crc.h
    ${CMAKE_SOURCE_DIR}/public/tier1/checksum_md5.h
    ${CMAKE_SOURCE_DIR}/public/tier1/strtools.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlbuffer.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utllinkedlist.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlmemory.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlrbtree.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlsymbol.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlvector.h
    ${CMAKE_SOURCE_DIR}/public/vcollide.h
    ${CMAKE_SOURCE_DIR}/public/vstdlib/vstdlib.h
    ${CMAKE_SOURCE_DIR}/public/wadtypes.h
    ../common/ISQLDBReplyTarget.h
    #../common/MySqlDatabase.h
    ../common/bsplib.h
    ../common/cmdlib.h
    ../common/pacifier.h
    ../common/scriplib.h
    ../common/threads.h
    vis.h
)
