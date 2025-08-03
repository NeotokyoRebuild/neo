add_library(vrad_library SHARED)

target_include_directories(vrad_library
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/common
    ${CMAKE_SOURCE_DIR}/utils/vmpi
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
)

if(OS_WINDOWS)
    target_link_libraries(vrad_library
        PRIVATE
        ws2_32.lib
    )

    if(NEO_VMPI)
        target_link_libraries(vrad_library
            PRIVATE
            vmpi::vmpi
        )
    endif()
endif()

target_sources_grouped(
    TARGET vrad_library
    NAME "Source Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/BSPTreeData.cpp
    ${CMAKE_SOURCE_DIR}/public/disp_common.cpp
    ${CMAKE_SOURCE_DIR}/public/disp_powerinfo.cpp
    ${CMAKE_SOURCE_DIR}/public/loadcmdline.cpp
    ${CMAKE_SOURCE_DIR}/public/lumpfiles.cpp
    ${CMAKE_SOURCE_DIR}/public/zip_utils.cpp
    ../common/MySqlDatabase.cpp
    ../common/pacifier.cpp
    ../common/physdll.cpp
    ../common/utilmatlib.cpp
    SampleHash.cpp
    VRAD_DispColl.cpp
    VRadDisps.cpp
    VRadStaticProps.cpp
    VradDetailProps.cpp
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

if(OS_WINDOWS)
    target_sources_grouped(
        TARGET vrad_library
        NAME "Source Files"
        FILES
        ../common/mpi_stats.cpp
        ../common/vmpi_tools_shared.cpp
        ../common/vmpi_tools_shared.h
        mpivrad.cpp

        # These files were [!$WIN32] in vpc
        ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
        ../common/filesystem_tools.cpp
    )
endif()

target_sources_grouped(
    TARGET vrad_library
    NAME "Common Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/ChunkFile.cpp
    ${CMAKE_SOURCE_DIR}/public/DispColl_Common.cpp
    ${CMAKE_SOURCE_DIR}/public/builddisp.cpp
    ../common/bsplib.cpp
    ../common/cmdlib.cpp
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
    ${CMAKE_SOURCE_DIR}/public/CollisionUtils.cpp
    ${CMAKE_SOURCE_DIR}/public/ScratchPad3D.cpp
    ${CMAKE_SOURCE_DIR}/public/ScratchPadUtils.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.cpp
)

target_sources_grouped(
    TARGET vrad_library
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/bitmap/tgawriter.h
    ${CMAKE_SOURCE_DIR}/public/map_utils.h
    VRAD_DispColl.h
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

if(OS_WINDOWS)
    target_sources_grouped(
        TARGET vrad_library
        NAME "Header Files"
        FILES
        mpivrad.h
    )
endif()

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

if(OS_WINDOWS)
    target_sources_grouped(
        TARGET vrad_library
        NAME "Common Header Files"
        FILES
        ../common/mpi_stats.h
        ../vmpi/ichannel.h
        ../vmpi/imysqlwrapper.h
        ../vmpi/iphelpers.h
        ../vmpi/messbuf.h
        ../vmpi/threadhelpers.h
        ../vmpi/vmpi_defs.h
        ../vmpi/vmpi_dispatch.h
        ../vmpi/vmpi_distribute_work.h
        ../vmpi/vmpi_filesystem.h
    )
endif()

target_sources_grouped(
    TARGET vrad_library
    NAME "Public Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/BSPFILE.H
    ${CMAKE_SOURCE_DIR}/public/BSPTreeData.h
    ${CMAKE_SOURCE_DIR}/public/ChunkFile.h
    ${CMAKE_SOURCE_DIR}/public/CollisionUtils.h
    ${CMAKE_SOURCE_DIR}/public/DispColl_Common.h
    ${CMAKE_SOURCE_DIR}/public/GameBSPFile.h
    ${CMAKE_SOURCE_DIR}/public/ScratchPad3D.h
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
    ${CMAKE_SOURCE_DIR}/public/mathlib/ANORMS.H
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
