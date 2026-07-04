add_library(vrad_library SHARED)

set_target_properties(vrad_library PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
)

if(OS_LINUX)
    set_target_properties(vrad_library PROPERTIES
        OUTPUT_NAME "vrad"
    )
endif()

if(NEO_USE_SEPARATE_BUILD_INFO)
    split_debug_information(TARGET vrad_library)
endif()

add_library_copy_target(TARGET vrad_library)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
    add_dependencies(vrad_library_copy_lib vrad_library_split_debug_information)
endif()

target_include_directories(vrad_library
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/common
)

target_compile_definitions(vrad_library
    PRIVATE
    PROTECTED_THINGS_DISABLE
    VRAD
)

target_link_libraries(vrad_library
    PRIVATE
    mathlib::mathlib
    tier2::tier2
    lzma::lzma
    bitmap::bitmap
    raytrace::raytrace
    vtf::vtf
    vstdlib::vstdlib
    Threads::Threads
)

if(OS_WINDOWS)
    target_link_libraries(vrad_library
        PRIVATE
        ws2_32.lib
    )
elseif(OS_LINUX)
    set_target_properties(vrad_library PROPERTIES
        OUTPUT_NAME "vrad"
    )
endif()

set_source_files_properties(${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp PROPERTIES
    COMPILE_OPTIONS "-D_DLL_EXT=${SRV_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX};-D_EXTERNAL_DLL_EXT=${SRV_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Source Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/bsptreedata.cpp
    ${CMAKE_SOURCE_DIR}/public/disp_common.cpp
    ${CMAKE_SOURCE_DIR}/public/disp_powerinfo.cpp
    ${CMAKE_SOURCE_DIR}/public/loadcmdline.cpp
    ${CMAKE_SOURCE_DIR}/public/lumpfiles.cpp
    ${CMAKE_SOURCE_DIR}/public/zip_utils.cpp
    ../common/pacifier.cpp
    ../common/physdll.cpp
    ../common/utilmatlib.cpp
    samplehash.cpp
    vrad_dispcoll.cpp
    vraddisps.cpp
    vradstaticprops.cpp
    vraddetailprops.cpp
    disp_vrad.cpp
    imagepacker.cpp
    incremental.cpp
    leaf_ambient_lighting.cpp
    lightmap.cpp
    macro_texture.cpp
    radial.cpp
    trace.cpp
    vismat.cpp
    vrad.cpp
    vraddll.cpp
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Common Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/chunkfile.cpp
    ${CMAKE_SOURCE_DIR}/public/dispcoll_common.cpp
    ${CMAKE_SOURCE_DIR}/public/builddisp.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
    ../common/bsplib.cpp
    ../common/cmdlib.cpp
    ../common/filesystem_tools.cpp
    ../common/map_shared.cpp
    ../common/polylib.cpp
    ../common/scriplib.cpp
    ../common/threads.cpp
    ../common/tools_minidump.cpp
    ../common/tools_minidump.h
    notes.txt
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Public Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/collisionutils.cpp
    ${CMAKE_SOURCE_DIR}/public/scratchpad3d.cpp
    ${CMAKE_SOURCE_DIR}/public/ScratchPadUtils.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.cpp
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/bitmap/tgawriter.h
    ${CMAKE_SOURCE_DIR}/public/map_utils.h
    vrad_dispcoll.h
    disp_vrad.h
    iincremental.h
    imagepacker.h
    incremental.h
    leaf_ambient_lighting.h
    lightmap.h
    macro_texture.h
    radial.h
    vismat.h
    vrad.h
    vraddetailprops.h
    vraddll.h
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Common Header Files"
    FILES
    ../common/ISQLDBReplyTarget.h
    ../common/MySqlDatabase.h
    ../common/bsplib.h
    ../common/cmdlib.h
    ../common/consolewnd.h
    ../common/map_shared.h
    ../common/pacifier.h
    ../common/polylib.h
    ../common/scriplib.h
    ../common/threads.h
    ../common/utilmatlib.h
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Public Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/bspfile.h
    ${CMAKE_SOURCE_DIR}/public/bsptreedata.h
    ${CMAKE_SOURCE_DIR}/public/chunkfile.h
    ${CMAKE_SOURCE_DIR}/public/collisionutils.h
    ${CMAKE_SOURCE_DIR}/public/dispcoll_common.h
    ${CMAKE_SOURCE_DIR}/public/gamebspfile.h
    ${CMAKE_SOURCE_DIR}/public/scratchpad3d.h
    ${CMAKE_SOURCE_DIR}/public/ScratchPadUtils.h
    ${CMAKE_SOURCE_DIR}/public/appframework/IAppSystem.h
    ${CMAKE_SOURCE_DIR}/public/basehandle.h
    ${CMAKE_SOURCE_DIR}/public/bitvec.h
    ${CMAKE_SOURCE_DIR}/public/bspflags.h
    ${CMAKE_SOURCE_DIR}/public/builddisp.h
    ${CMAKE_SOURCE_DIR}/public/cmodel.h
    ${CMAKE_SOURCE_DIR}/public/const.h
    ${CMAKE_SOURCE_DIR}/public/coordsize.h
    ${CMAKE_SOURCE_DIR}/public/disp_common.h
    ${CMAKE_SOURCE_DIR}/public/disp_powerinfo.h
    ${CMAKE_SOURCE_DIR}/public/disp_vertindex.h
    ${CMAKE_SOURCE_DIR}/public/filesystem.h
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.h
    ${CMAKE_SOURCE_DIR}/public/gametrace.h
    ${CMAKE_SOURCE_DIR}/public/ihandleentity.h
    ${CMAKE_SOURCE_DIR}/public/iscratchpad3d.h
    ${CMAKE_SOURCE_DIR}/public/ivraddll.h
    ${CMAKE_SOURCE_DIR}/public/materialsystem/hardwareverts.h
    ${CMAKE_SOURCE_DIR}/public/materialsystem/imaterial.h
    ${CMAKE_SOURCE_DIR}/public/materialsystem/imaterialsystem.h
    ${CMAKE_SOURCE_DIR}/public/materialsystem/imaterialvar.h
    ${CMAKE_SOURCE_DIR}/public/materialsystem/materialsystem_config.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/anorms.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/amd3dx.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/bumpvects.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/compressed_vector.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/halton.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/mathlib.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector2d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector4d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vmatrix.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vplane.h
    ${CMAKE_SOURCE_DIR}/public/optimize.h
    ${CMAKE_SOURCE_DIR}/public/phyfile.h
    ${CMAKE_SOURCE_DIR}/public/string_t.h
    ${CMAKE_SOURCE_DIR}/public/studio.h
    ${CMAKE_SOURCE_DIR}/public/tier0/basetypes.h
    ${CMAKE_SOURCE_DIR}/public/tier0/commonmacros.h
    ${CMAKE_SOURCE_DIR}/public/tier0/dbg.h
    ${CMAKE_SOURCE_DIR}/public/tier0/fasttimer.h
    ${CMAKE_SOURCE_DIR}/public/tier0/icommandline.h
    ${CMAKE_SOURCE_DIR}/public/tier0/memdbgon.h
    ${CMAKE_SOURCE_DIR}/public/tier0/platform.h
    ${CMAKE_SOURCE_DIR}/public/tier0/protected_things.h
    ${CMAKE_SOURCE_DIR}/public/tier0/vprof.h
    ${CMAKE_SOURCE_DIR}/public/tier1/bitbuf.h
    ${CMAKE_SOURCE_DIR}/public/tier1/byteswap.h
    ${CMAKE_SOURCE_DIR}/public/tier1/characterset.h
    ${CMAKE_SOURCE_DIR}/public/tier1/checksum_crc.h
    ${CMAKE_SOURCE_DIR}/public/tier1/checksum_md5.h
    ${CMAKE_SOURCE_DIR}/public/tier1/interface.h
    ${CMAKE_SOURCE_DIR}/public/tier1/strtools.h
    ${CMAKE_SOURCE_DIR}/public/tier1/tokenreader.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlbuffer.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utldict.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlhash.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utllinkedlist.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlmemory.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlrbtree.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlsymbol.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlvector.h
    ${CMAKE_SOURCE_DIR}/public/trace.h
    ${CMAKE_SOURCE_DIR}/public/vcollide.h
    ${CMAKE_SOURCE_DIR}/public/vphysics_interface.h
    ${CMAKE_SOURCE_DIR}/public/vstdlib/random.h
    ${CMAKE_SOURCE_DIR}/public/vstdlib/vstdlib.h
    ${CMAKE_SOURCE_DIR}/public/vtf/vtf.h
    ${CMAKE_SOURCE_DIR}/public/wadtypes.h
    ${CMAKE_SOURCE_DIR}/public/worldsize.h
    ../common/physdll.h
)
