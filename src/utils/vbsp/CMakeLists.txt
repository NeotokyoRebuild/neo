#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_executable(vbsp)

target_include_directories(vbsp
    PRIVATE
    ${CMAKE_SOURCE_DIR}/utils
    ${CMAKE_SOURCE_DIR}/utils/common
    ${CMAKE_SOURCE_DIR}/utils/vmpi
)

target_compile_definitions(vbsp
    PRIVATE
    MACRO_MATHLIB
    PROTECTED_THINGS_DISABLE
)

if(OS_WINDOWS)
    target_compile_options(vbsp
        PRIVATE
        /wd4091
        /wd4316
        /wd4355
        /wd4577
        /wd5033
        /wd5054
        /wd5055
    )

    target_link_libraries(vbsp
        PRIVATE
        ws2_32.lib
        odbc32.lib
        odbccp32.lib
        winmm.lib
    )
elseif(OS_LINUX)
    target_compile_definitions(vbsp
        PRIVATE
        #GL_GLEXT_PROTOTYPES
        #DX_TO_GL_ABSTRACTION
        USE_SDL
        ALLOW_NOSHADERAPI
    )

    # target_compile_options(vbsp
    #     PRIVATE
    #     -gdwarf-4
    # )

    # set_target_properties(vbsp PROPERTIES
    #     #BUILD_RPATH "$ORIGIN"
    #     #BUILD_RPATH_USE_ORIGIN TRUE

    #     #BUILD_WITH_INSTALL_RPATH TRUE
    # )

    # target_link_options(vbsp
    #     PRIVATE
    #     -Wl,--disable-new-dtags
    #     -Wl,-rpath,$ORIGIN
    #     -Wl,-z,origin
    #     #-Wl,-rpath=.
    # )
endif()

target_link_libraries(vbsp
    PRIVATE
    fgdlib::fgdlib
    mathlib::mathlib
    tier2::tier2
    vtf::vtf
    bitmap::bitmap
    lzma::lzma
    vstdlib::vstdlib
)

target_sources_grouped(
    TARGET vbsp
    NAME "Source Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/collisionutils.cpp
    ${CMAKE_SOURCE_DIR}/public/disp_common.cpp
    ${CMAKE_SOURCE_DIR}/public/disp_powerinfo.cpp
    ${CMAKE_SOURCE_DIR}/public/loadcmdline.cpp
    ${CMAKE_SOURCE_DIR}/public/lumpfiles.cpp
    ${CMAKE_SOURCE_DIR}/public/scratchpad3d.cpp
    ${CMAKE_SOURCE_DIR}/public/zip_utils.cpp
    ../common/mstristrip.cpp
    ../common/physdll.cpp
    ../common/scratchpad_helpers.cpp
    ../common/utilmatlib.cpp
    boundbox.cpp
    brushbsp.cpp
    csg.cpp
    cubemap.cpp
    detail.cpp
    detailobjects.cpp
    disp_ivp.cpp
    disp_vbsp.cpp
    faces.cpp
    glfile.cpp
    ivp.cpp
    leakfile.cpp
    manifest.cpp
    map.cpp
    materialpatch.cpp
    materialsub.cpp
    nodraw.cpp
    normals.cpp
    overlay.cpp
    portals.cpp
    prtfile.cpp
    staticprop.cpp
    textures.cpp
    tree.cpp
    vbsp.cpp
    worldvertextransitionfixup.cpp
    writebsp.cpp
)

target_sources_grouped(
    TARGET vbsp
    NAME "Common Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/builddisp.cpp
    ${CMAKE_SOURCE_DIR}/public/chunkfile.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.cpp
    ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
    ../common/bsplib.cpp
    ../common/cmdlib.cpp
    ../common/filesystem_tools.cpp
    ../common/map_shared.cpp
    ../common/pacifier.cpp
    ../common/polylib.cpp
    ../common/scriplib.cpp
    ../common/threads.cpp
    ../common/tools_minidump.cpp
    ../common/tools_minidump.h
)

target_sources_grouped(
    TARGET vbsp
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/disp_powerinfo.h
    ${CMAKE_SOURCE_DIR}/public/disp_vertindex.h
    ../common/scratchpad_helpers.h
    boundbox.h
    csg.h
    detail.h
    disp_vbsp.h
    faces.h
    manifest.h
    map.h
    materialpatch.h
    materialsub.h
    vbsp.h
    worldvertextransitionfixup.h
    writebsp.h
)

target_sources_grouped(
    TARGET vbsp
    NAME "Common header files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/builddisp.h
    ${CMAKE_SOURCE_DIR}/public/chunkfile.h
    ${CMAKE_SOURCE_DIR}/public/filesystem.h
    ${CMAKE_SOURCE_DIR}/public/filesystem_helpers.h
    ${CMAKE_SOURCE_DIR}/public/gamebspfile.h
    ${CMAKE_SOURCE_DIR}/public/tier1/interface.h
    ${CMAKE_SOURCE_DIR}/public/tier1/tokenreader.h
    ${CMAKE_SOURCE_DIR}/public/zip_uncompressed.h
    ../common/bsplib.h
    ../common/cmdlib.h
    ../common/filesystem_tools.h
    ../common/map_shared.h
    ../common/pacifier.h
    ../common/polylib.h
    ../common/utilmatlib.h
    ../vmpi/vmpi.h
    disp_ivp.h
    ivp.h
)

target_sources_grouped(
    TARGET vbsp
    NAME "Public Headers"
    FILES
    ${CMAKE_SOURCE_DIR}/public/arraystack.h
    ${CMAKE_SOURCE_DIR}/public/bspfile.h
    ${CMAKE_SOURCE_DIR}/public/bspflags.h
    ${CMAKE_SOURCE_DIR}/public/bsptreedata.h
    #${CMAKE_SOURCE_DIR}/public/btree.h
    ${CMAKE_SOURCE_DIR}/public/cmodel.h
    ${CMAKE_SOURCE_DIR}/public/collisionutils.h
    ${CMAKE_SOURCE_DIR}/public/disp_common.h
    ${CMAKE_SOURCE_DIR}/public/iscratchpad3d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/amd3dx.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/bumpvects.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/mathlib.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector2d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vector4d.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vmatrix.h
    ${CMAKE_SOURCE_DIR}/public/mathlib/vplane.h
    ${CMAKE_SOURCE_DIR}/public/nmatrix.h
    ${CMAKE_SOURCE_DIR}/public/nvector.h
    ${CMAKE_SOURCE_DIR}/public/phyfile.h
    ${CMAKE_SOURCE_DIR}/public/scratchpad3d.h
    ${CMAKE_SOURCE_DIR}/public/studio.h
    ${CMAKE_SOURCE_DIR}/public/tier0/basetypes.h
    ${CMAKE_SOURCE_DIR}/public/tier0/commonmacros.h
    ${CMAKE_SOURCE_DIR}/public/tier0/dbg.h
    ${CMAKE_SOURCE_DIR}/public/tier1/byteswap.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlbuffer.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utllinkedlist.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlmemory.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlrbtree.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlsymbol.h
    ${CMAKE_SOURCE_DIR}/public/tier1/utlvector.h
    ${CMAKE_SOURCE_DIR}/public/vcollide.h
    ${CMAKE_SOURCE_DIR}/public/vphysics_interface.h
    ${CMAKE_SOURCE_DIR}/public/wadtypes.h
    ${CMAKE_SOURCE_DIR}/public/worldsize.h
    ../common/mstristrip.h
    ../common/physdll.h
    ../common/qfiles.h
    ../common/scriplib.h
    ../common/threads.h
)

target_sources_grouped(
    TARGET vbsp
    NAME "Other files"
    FILES
    "notes.txt"
)
