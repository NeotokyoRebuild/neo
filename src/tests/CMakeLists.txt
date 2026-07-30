
# To run all the unit tests:
#    $ ctest --test-dir build/[PRESET]/tests/ --output-on-failure
# To run unit tests executable matching regex:
#    $ ctest --test-dir build/[PRESET]/tests/ --output-on-failure -R [REGEX]
# To directly run the test executables:
#    Windows:
#        Make sure tier0.dll is in PATH ([SOURCE]\devtools\bin\), then do:
#        $ build\windows-[debug/release]\tests\test_....
#    Linux:
#        $ LD_LIBRARY_PATH=lib/public/linux64/ ./build/linux-[debug/release]/tests/test_....

include(CTest)

add_executable(test_neo_crosshair
    test_neo_crosshair.cpp
    test_util.h
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_crosshair.cpp
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_crosshair.h
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_serial.cpp
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_serial.h
)
add_executable(test_neo_serial
    test_neo_serial.cpp
    test_util.h
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_crosshair.h
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_serial.cpp
    ${CMAKE_SOURCE_DIR}/game/shared/neo/neo_serial.h
)

set(TEST_EXE_LIST
    test_neo_crosshair
    test_neo_serial
)

foreach (TEST_EXE ${TEST_EXE_LIST})
    add_test(NAME ${TEST_EXE}
        COMMAND $<TARGET_FILE:${TEST_EXE}>)

    if (OS_LINUX)
        set_property(TEST ${TEST_EXE}
            PROPERTY
                ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_SOURCE_DIR}/lib/public/linux64/"
        )
    elseif (OS_WINDOWS)
        set_property(TEST ${TEST_EXE}
            APPEND
            PROPERTY
                ENVIRONMENT "PATH=${CMAKE_SOURCE_DIR}\\devtools\\bin\\"
        )
    endif ()

    target_compile_definitions(${TEST_EXE}
        PRIVATE
        UNIT_TEST_DLL
        DBGFLAG_ASSERTFATAL
    )

    target_include_directories(${TEST_EXE}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/game/client
        ${CMAKE_SOURCE_DIR}/game/shared
        ${CMAKE_SOURCE_DIR}/game/shared/hl2
        ${CMAKE_SOURCE_DIR}/game/shared/hl2mp
        ${CMAKE_SOURCE_DIR}/game/shared/Multiplayer
        ${CMAKE_SOURCE_DIR}/game/shared/neo
        ${CMAKE_SOURCE_DIR}/game/shared/neo/weapons
        ${CMAKE_SOURCE_DIR}/public
        ${CMAKE_SOURCE_DIR}/thirdparty/sixensesdk/include
        ${CMAKE_SOURCE_DIR}/vendor
        ${CMAKE_SOURCE_DIR}/game/client/NextBot
    )

    target_link_libraries(${TEST_EXE}
        PRIVATE
        appframework::appframework
        bitmap::bitmap
        mathlib::mathlib
        choreo_objects::choreo_objects
        dmx_loader::dmx_loader
        tier0::tier0
        tier1::tier1
        tier2::tier2
        tier3::tier3
        matsys_controls::matsys_controls
        vstdlib::vstdlib
        particles::particles
        vtf::vtf
        steam_api::steam_api
        vgui_controls::vgui_controls
    )

    target_link_directories(${TEST_EXE}
        PRIVATE
        ${LIBCOMMON}
        ${LIBPUBLIC}
    )

    if(NEO_STAGING_ONLY)
        target_compile_definitions(${TEST_EXE} PRIVATE STAGING_ONLY)
    endif()

    if(NEO_RELEASE_ASSERTS)
        target_compile_definitions(${TEST_EXE} PRIVATE RELEASEASSERTS)
    endif()

    if(NEO_RETAIL)
        target_compile_definitions(${TEST_EXE} PRIVATE RETAIL)
    endif()

    if(OS_LINUX OR OS_MACOS)
        target_compile_definitions(${TEST_EXE}
            PRIVATE
            GL_GLEXT_PROTOTYPES
            DX_TO_GL_ABSTRACTION
            USE_SDL
        )
    endif()

    if(NEO_BUILD_WEAPON_PBK56S)
        target_compile_definitions(${TEST_EXE} PRIVATE INCLUDE_WEP_PBK)
    endif()

    if(OS_WINDOWS)
        if(NEO_BUILD_REPLAY)
            target_compile_definitions(${TEST_EXE} PRIVATE CURL_STATICLIB)

            target_link_libraries(${TEST_EXE}
                PRIVATE
                wsock32.lib
                Ws2_32.lib
            )
        endif()

        target_compile_definitions(${TEST_EXE}
            PRIVATE
            AVI_VIDEO
            WMV_VIDEO
            fopen=dont_use_fopen
            PSAPI_VERSION=1
        )

        target_link_libraries(${TEST_EXE}
            PRIVATE
            legacy_stdio_definitions.lib
            #winmm.lib
            libz
            Psapi.lib
        )
    endif()

    if(OS_LINUX)
        target_compile_definitions(${TEST_EXE}
            PRIVATE
            LINUX
            _LINUX
            BINK_VIDEO
            USE_WEBM_FOR_REPLAY
        )

        target_link_libraries(${TEST_EXE}
            PRIVATE
            #${CMAKE_SOURCE_DIR}/lib/common/linux64/libcrypto.a
            #${CMAKE_SOURCE_DIR}/lib/common/linux64/libssl.a
            #${LIBCOMMON}/libcurl.a
            #${LIBCOMMON}/libcurlssl.a
            vgui_controls::vgui_controls
            dl
            m
            pthread
            rt
            z
            GL
        )
    endif()
endforeach (TEST_EXE)

