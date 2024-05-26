cmake_minimum_required(VERSION 3.18.4)

cmake_policy(SET CMP0091 NEW)

project(neo)
set(CMAKE_VERBOSE_MAKEFILE TRUE)
message(STATUS "CMAKE_VERSION: ${CMAKE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(cmake/utils.cmake)

if(NOT OS_WINDOWS AND NOT OS_LINUX AND NOT OS_MACOS)
    message(FATAL_ERROR "Unknown OS")
endif()

option(NEO_CHROOT_BUILD "chroot build mode" OFF)
option(NEO_USE_CCACHE "Use ccache" ${OS_LINUX})
option(NEO_USE_VALVE_BINDIR "Use /valve bindir" OFF) # NOTE probably should be used with NEO_DEDICATED
option(NEO_STAGING_ONLY "Staging mode (STAGING_ONLY)" OFF)
option(NEO_RELEASE_ASSERTS "Release asserts (RELEASEASSERTS)" OFF)
option(NEO_RETAIL "Retail mode" OFF)
option(NEO_DEDICATED "Dedicated server mode" OFF)
option(NEO_RAD_TELEMETRY_DISABLED "Telemetry disabled" ON)
option(NEO_QUICKTIME_VIDEO "Use Quicktime video on Windows" OFF)
option(NEO_BUILD_REPLAY "Build replay system" OFF) # NOTE not implemented for neo
option(NEO_BUILD_WEAPON_PBK56S "Build PBK56S weapon" OFF)
option(NEO_COPY_LIBRARIES "Copy libraries to bin directory by default" ON)
set(NEO_OUTPUT_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/../game/neo/bin" CACHE PATH "Directory for output libraries")

if(OS_WINDOWS)
    set(NEO_BUILD_TIER1 ON CACHE BOOL "" FORCE)
    set(NEO_BUILD_MATHLIB ON CACHE BOOL "" FORCE)
    set(NEO_BUILD_VGUI_CONTROLS ON CACHE BOOL "" FORCE)
else()
    option(NEO_BUILD_TIER1 "Build tier1 library" ON)
    option(NEO_BUILD_MATHLIB "Build mathlib library" ON)
    option(NEO_BUILD_VGUI_CONTROLS "Build vgui_controls library" ON)
endif()

message(STATUS "chroot build mode: ${NEO_CHROOT_BUILD}")
message(STATUS "Use ccache: ${NEO_USE_CCACHE}")
message(STATUS "Use /valve bindir: ${NEO_USE_VALVE_BINDIR}")
message(STATUS "Staging (STAGING_ONLY): ${NEO_STAGING_ONLY}")
message(STATUS "Release asserts (RELEASEASSERTS): ${NEO_RELEASE_ASSERTS}")
message(STATUS "Retail mode (RETAIL): ${NEO_RETAIL}")
message(STATUS "Dedicated server mode (DEDICATED): ${NEO_DEDICATED}")
message(STATUS "Telemetry disabled (RAD_TELEMETRY_DISABLED): ${NEO_RAD_TELEMETRY_DISABLED}")
message(STATUS "Use Quicktime video on Windows (QUICKTIME_VIDEO): ${NEO_QUICKTIME_VIDEO}")
message(STATUS "Build replay system (BUILD_REPLAY): ${NEO_BUILD_REPLAY}")
message(STATUS "Build PBK56S weapon (INCLUDE_WEP_PBK): ${NEO_BUILD_WEAPON_PBK56S}")
message(STATUS "Build tier1 library: ${NEO_BUILD_TIER1}")
message(STATUS "Build mathlib library: ${NEO_BUILD_MATHLIB}")
message(STATUS "Build vgui_controls library: ${NEO_BUILD_VGUI_CONTROLS}")
message(STATUS "Copy libraries to bin directory by default: ${NEO_COPY_LIBRARIES}")
message(STATUS "Directory for output libraries: ${NEO_OUTPUT_LIBRARY_PATH}")


file(MAKE_DIRECTORY "${NEO_OUTPUT_LIBRARY_PATH}")

if(CMAKE_VERBOSE_MAKEFILE)
    set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

set(SOURCESDK TRUE)

# TODO use base.xcconfig for OS_MACOS

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
        DEBUG
        _DEBUG
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(
        NDEBUG
    )

    add_compile_options(
        #-fno-strict-aliasing
        -ffast-math
        -fno-omit-frame-pointer
        -ftree-vectorize
    )
endif()

if(NEO_RAD_TELEMETRY_DISABLED)
    add_compile_definitions(RAD_TELEMETRY_DISABLED)
endif()

if(NEO_CHROOT_BUILD)
    set(STEAM_RUNTIME_PATH /usr)
    set(CRYPTOPPDIR ubuntu12_32_gcc48)
elseif(NEO_USE_VALVE_BINDIR)
    set(STEAM_RUNTIME_PATH /valve)
    set(CRYPTOPPDIR linux32)
else()
    set(STEAM_RUNTIME_PATH /usr)
    set(CRYPTOPPDIR ubuntu12_32)
endif()

add_compile_definitions(
    SDK2013CE
    NEO
    _DLL_EXT=${CMAKE_SHARED_LIBRARY_SUFFIX}
    _EXTERNAL_DLL_EXT=${CMAKE_SHARED_LIBRARY_SUFFIX}
    _GLIBCXX_USE_CXX11_ABI=0
    FRAME_POINTER_OMISSION_DISABLED
    SOURCE1=1
    DEV_BUILD
)

if(OS_WINDOWS)
    set(LIBPUBLIC "${CMAKE_SOURCE_DIR}/lib/public")
    set(LIBCOMMON "${CMAKE_SOURCE_DIR}/lib/common")

    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "EditAndContinue")

    # Disable exceptions
    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    add_compile_options(
        /arch:SSE2
        /EHs-c-
        /fp:fast # FloatingPointModel - Fast
        /FC # UseFullPaths
        /errorReport:prompt # ErrorReporting - Prompt Immediately
        /W4
        /Gw # Optimize Global Data

        #/wd4316
        #/wd4456
        #/wd4459
        #/wd4457
        #/wd4838
        #/wd26945
        #/wd4458
        #/wd4559
        #/wd4565
    )

    add_compile_definitions(
        WIN32
        _WIN32
        _WINDOWS
        COMPILER_MSVC
        COMPILER_MSVC32
        _LIB
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_DEPRECATE
        _ALLOW_RUNTIME_LIBRARY_MISMATCH
        _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH
        _ALLOW_MSC_VER_MISMATCH
        fopen=dont_use_fopen
        _MBCS
    )

    add_link_options(
        /DYNAMICBASE:NO
        /NODEFAULTLIB:libc
        /NODEFAULTLIB:libcd
    )

    link_libraries(
        legacy_stdio_definitions.lib
        uuid.lib
    )

    set(CMAKE_MSVC_RUNTIME_LIBRARY "")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(
            #/Wp64 # Detect64bitPortabilityIssues
            #/Od # Optimization - Disabled
            /GF # EnableStringPooling
            /Zc:wchar_t
            /Zc:forScope
            /Zc:inline
            /Zc:forScope
            /Oy-
            /MTd
        )

        add_compile_definitions(
            _HAS_ITERATOR_DEBUGGING=0
        )

        add_link_options(
            /NODEFAULTLIB:libcmt
            /NODEFAULTLIB:libcpmt
            /NODEFAULTLIB:libcpmt1
        )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        # TODO "Use Link Time Code Generation" [$LTCG] ?
        # Also /GL https://learn.microsoft.com/en-us/cpp/build/reference/gl-whole-program-optimization

        add_compile_options(
            #/Z7 # DebugInformationFormat - C7 Compatible
            #/Wp64 # Detect64bitPortabilityIssues
            #/O2 # Optimization - Maximize Speed
            /Ob2 # InlineFunctionExpansion - Any Suitable
            /Oi # EnableIntrinsicFunctions
            /Ot # FavorSizeOrSpeed - Favor Fast Code
            /GF # EnableStringPooling
            /Gy # EnableFunctionLevelLinking
            #/Zo
            /Oy-
            /MT
        )

        add_link_options(
            /NODEFAULTLIB:libcmtd
            /NODEFAULTLIB:libcpmtd
            /NODEFAULTLIB:libcpmtd0
            /NODEFAULTLIB:libcpmtd1
        )
    endif()
endif()

if(OS_LINUX OR OS_MACOS)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)

    # Set default optimization option to O2 instead of O3
    string(REGEX REPLACE "([\\/\\-]O)3" "\\12" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "([\\/\\-]O)3" "\\12" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REGEX REPLACE "([\\/\\-]O)3" "\\12" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    string(REGEX REPLACE "([\\/\\-]O)3" "\\12" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    # Disable exceptions
    string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    add_compile_definitions(
        GNUC
        POSIX
        _POSIX
        NO_HOOK_MALLOC
        NO_MALLOC_OVERRIDE
        strncpy=use_Q_strncpy_instead
        _snprintf=use_Q_snprintf_instead
        VPROF_LEVEL=1 # TODO check value TODO only for MACOS
    )

    add_compile_options(
        -m32
        -fno-exceptions
        -msse2

        -Wall
        #-Wextra # if STEAM_BRANCH
        #-Wshadow # if STEAM_BRANCH

        $<$<CXX_COMPILER_ID:GNU>:-mfpmath=sse>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-overloaded-virtual>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
        -Wno-multichar
        -Wno-write-strings
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-function
        -Wno-unknown-pragmas
        -Wno-unused-parameter
        -Wno-unused-value
        -Wno-missing-field-initializers
        -Wno-sign-compare
        -Wno-float-equal
        -Wno-unused-local-typedefs
        -Wno-switch
        -Wno-strict-aliasing

        -Werror=return-type
        -fdiagnostics-show-option
        -Wformat -Wformat-security


        -Usprintf
        -Ustrncpy

        # Fix for non-clickable vgui buttons when using -O2
        -fno-devirtualize-speculatively

        # Fix for non-clickable vgui buttons when using -O3
        -fno-ipa-cp-clone
    )

    # Disable option for older compiler in docker
    if(NOT CMAKE_CXX_COMPILER_VERSION STREQUAL "4.8.4")
        add_compile_options(
            $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-fabi-compat-version=2>
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess>
            $<$<COMPILE_LANGUAGE:CXX>:-Wno-template-id-cdtor>
            -Wno-nonnull-compare
        )
    endif()

    # We should always specify -Wl,--build-id, as documented at:
    # http://linux.die.net/man/1/ld and http://fedoraproject.org/wiki/Releases/FeatureBuildId.http://fedoraproject.org/wiki/Releases/FeatureBuildId
    add_link_options(
        -m32
        -Wl,--build-id
        -static-libgcc
        -Wl,--no-undefined
        -Wl,--version-script=${CMAKE_SOURCE_DIR}/devtools/version_script.linux.txt
    )

    # Fix undefined references in static libraries
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LANGUAGE_COMPILE_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> -Wl,--start-group <LINK_LIBRARIES> -Wl,--end-group")
endif()

if(OS_LINUX)
    set(LIBPUBLIC "${CMAKE_SOURCE_DIR}/lib/public/linux32")
    set(LIBCOMMON "${CMAKE_SOURCE_DIR}/lib/common/linux32")

    add_compile_definitions(
        LINUX
        _LINUX
    )
endif()

if(OS_MACOS)
    set(LIBPUBLIC "${CMAKE_SOURCE_DIR}/lib/public/osx32")
    set(LIBCOMMON "${CMAKE_SOURCE_DIR}/lib/common/osx32")
endif()

if(NOT NEO_DEDICATED AND (OS_LINUX OR OS_MACOS))
    add_compile_definitions(
        GL_GLEXT_PROTOTYPES
        DX_TO_GL_ABSTRACTION
        USE_SDL
    )
endif()

if(COMPILER_MSVC)
    add_compile_definitions(COMPILER_MSVC)
elseif(COMPILER_GCC)
    add_compile_definitions(COMPILER_GCC)
elseif(COMPILER_CLANG)
    add_compile_definitions(COMPILER_CLANG)
endif()

if(NEO_USE_VALVE_BINDIR)
    # On dedicated servers, some plugins depend on global variable symbols in addition to functions.
	# So symbols like _Z16ClearMultiDamagev should show up when you do "nm server_srv.so" in TF2.

    # TODO disable strip
endif()

if(NEO_USE_CCACHE)
    find_program(CCACHE_PATH
        NAMES ccache
        PATHS ${CMAKE_SOURCE_DIR}/devtools/bin/linux
        #NO_DEFAULT_PATH
        REQUIRED
    )

    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PATH})

    # http://petereisentraut.blogspot.com/2011/05/ccache-and-clang.html
    add_compile_options(
        #-Qunused-arguments
        #-fcolor-diagnostics
    )
endif()

# TODO create separate debug info?
# devtools/gendbg.sh

# TODO add option to create _srv copies?

add_subdirectory(lib)

if(NOT NEO_DEDICATED)
    add_subdirectory(game/client)
    add_subdirectory(materialsystem/stdshaders)
endif()

add_subdirectory(game/server)

if(NEO_COPY_LIBRARIES)
    set(COPY_ALL_LIBS_FLAG ALL)
else()
    unset(COPY_ALL_LIBS_FLAG)
endif()

add_custom_target(
    copy_all_libs ${COPY_ALL_LIBS_FLAG}
    DEPENDS client_copy_lib server_copy_lib game_shader_dx9_copy_lib
    VERBATIM
)

# TODO add later?
# $Project "raytrace" "raytrace\raytrace.vpc" [$WIN32||$X360||$POSIX]

# Not needed?
# $Project "serverplugin_empty" "utils\serverplugin_sample\serverplugin_empty.vpc" [$WIN32||$POSIX]
# $Project "captioncompiler" "utils\captioncompiler\captioncompiler.vpc" [$WIN32]
# $Project "fgdlib" "fgdlib\fgdlib.vpc" [$WIN32]
# $Project "glview" "utils\glview\glview.vpc" [$WIN32]
# $Project "height2normal" "utils\height2normal\height2normal.vpc" [$WIN32]
# $Project "motionmapper" "utils\motionmapper\motionmapper.vpc" [$WIN32]
# $Project "phonemeextractor" "utils\phonemeextractor\phonemeextractor.vpc" [$WIN32]
# $Project "qc_eyes" "utils\qc_eyes\qc_eyes.vpc" [$WIN32]
# $Project "tgadiff" "utils\tgadiff\tgadiff.vpc" [$WIN32]
# $Project "vbsp" "utils\vbsp\vbsp.vpc" [$WIN32]
# $Project "vice" "utils\vice\vice.vpc" [$WIN32]
# $Project "vrad_dll" "utils\vrad\vrad_dll.vpc" [$WIN32]
# $Project "vrad_launcher" "utils\vrad_launcher\vrad_launcher.vpc" [$WIN32]
# $Project "vtf2tga" "utils\vtf2tga\vtf2tga.vpc" [$WIN32]
# $Project "vtfdiff" "utils\vtfdiff\vtfdiff.vpc" [$WIN32]
# $Project "vvis_dll" "utils\vvis\vvis_dll.vpc" [$WIN32]
# $Project "vvis_launcher" "utils\vvis_launcher\vvis_launcher.vpc" [$WIN32]
