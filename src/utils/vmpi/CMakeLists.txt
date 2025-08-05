add_library(vmpi STATIC)

add_library(vmpi::vmpi ALIAS vmpi)

set_target_properties(vmpi PROPERTIES PREFIX "")

target_include_directories(vmpi
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils/common
    ${CMAKE_SOURCE_DIR}/thirdparty/libcurl/include
    ZLib
)

target_compile_definitions(vmpi
    PRIVATE
    PROTECTED_THINGS_DISABLE
)

if(NEO_VMPI)
    target_compile_definitions(vmpi
        PUBLIC
        MPI
    )
endif()

target_link_libraries(vmpi
    PUBLIC
    ZLib.lib
    CURL::libcurl_static

    PRIVATE
    tier1::tier1
    vstdlib::vstdlib
    libz::libz
)

target_link_directories(vmpi
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources_grouped(
    TARGET vmpi
    NAME "Source Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/filesystem_init.cpp
    ../common/filesystem_tools.cpp
    ThreadedTCPSocket.cpp
    ThreadedTCPSocketEmu.cpp
    iphelpers.cpp
    loopback_channel.cpp
    messbuf.cpp
    threadhelpers.cpp
    vmpi.cpp
    vmpi_distribute_tracker.cpp
    vmpi_distribute_work.cpp
    vmpi_distribute_work_default.cpp
    vmpi_distribute_work_sdk.cpp
    vmpi_filesystem.cpp
    vmpi_filesystem_internal.h
    vmpi_filesystem_master.cpp
    vmpi_filesystem_worker.cpp
    vmpi_logfile.cpp
    vmpi_logfile.h
)

target_sources_grouped(
    TARGET vmpi
    NAME "Header Files"
    FILES
    ${CMAKE_SOURCE_DIR}/public/tier1/bitbuf.h
    IThreadedTCPSocket.h
    ThreadedTCPSocketEmu.h
    ichannel.h
    iphelpers.h
    loopback_channel.h
    messbuf.h
    tcpsocket.h
    threadhelpers.h
    vmpi.h
    vmpi_defs.h
    vmpi_filesystem.h
)
