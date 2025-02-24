add_library(game_shader_dx9 SHARED)

set_target_properties(game_shader_dx9 PROPERTIES PREFIX "")

if(NEO_USE_SEPARATE_BUILD_INFO)
    split_debug_information(TARGET game_shader_dx9)
endif()

add_library_copy_target(TARGET game_shader_dx9)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
    add_dependencies(game_shader_dx9_copy_lib game_shader_dx9_split_debug_information)
endif()

target_include_directories(game_shader_dx9
    PRIVATE
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/public
        fxctmp9
        vshtmp9
	include
)

target_compile_definitions(game_shader_dx9
    PRIVATE
    FAST_MATERIALVAR_ACCESS
    GAME_SHADER_DLL
    STDSHADER_DX9_DLL_EXPORT
)

if(OS_WINDOWS)
    target_compile_definitions(game_shader_dx9
        PRIVATE
        USE_ACTUAL_DX
    )

    target_link_libraries(game_shader_dx9
        PRIVATE
        version.lib
        winmm.lib
        ${LIBPUBLIC}/shaderlib.lib
    )
elseif(OS_LINUX)
    target_link_libraries(game_shader_dx9
        PRIVATE
        ${LIBPUBLIC}/shaderlib.a
    )
elseif(OS_MACOS)

endif()

target_link_libraries(game_shader_dx9
    PRIVATE
    tier0::tier0
    tier1::tier1
    tier2::tier2
    mathlib::mathlib
    vstdlib::vstdlib
    matsys_controls::matsys_controls
)

target_link_directories(game_shader_dx9
    PRIVATE
    ${LIBCOMMON}
    ${LIBPUBLIC}
)

# TODO
#$File buildsdkshaders.bat
#$File buildshaders.bat

#$Shaders	stdshader_dx9_20b.txt
#$Shaders	stdshader_dx9_30.txt

target_sources_grouped(
    TARGET game_shader_dx9
    NAME "Source Files"
    FILES
    BaseVSShader.cpp
    Bloom.cpp
    example_model_dx9.cpp
    example_model_dx9_helper.cpp
    screenspace_general.cpp
)

if(OS_LINUX)
    target_sources_grouped(
        TARGET game_shader_dx9
        NAME "Source Files"
        FILES
        ${CMAKE_SOURCE_DIR}/game/shared/math_finite_fix.cpp
    )
endif()

target_sources_grouped(
    TARGET game_shader_dx9
    NAME "Header Files"
    FILES
    BaseVSShader.h
    common_fxc.h
    common_hlsl_cpp_consts.h
    common_ps_fxc.h
    common_vertexlitgeneric_dx9.h
    common_vs_fxc.h
    example_model_dx9_helper.h
    shader_constant_register_map.h
)

target_sources_grouped(
    TARGET game_shader_dx9
    NAME "Source Files"
    FILES
    #Neo_MotionVision_ModelShader.cpp
    Neo_GaussianBlur_Shader.cpp
    Neo_MotionVision_Shader_Pass2.cpp
    neo_motion_model_dx9.cpp
    neo_motion_model_dx9_helper.cpp
    neo_motion_model_dx9_helper.h
    Neo_NightVision_Shader.cpp
    Neo_Noise_Shader.cpp
    Neo_SSAO_Combine_Shader.cpp
    Neo_SSAO_Shader.cpp
    Neo_TestShader.cpp
    Neo_ThermalVision.cpp
    neo_thermal_model_dx9.cpp
    neo_thermal_model_dx9_helper.cpp
    neo_thermal_model_dx9_helper.h
    Neo_colorblind.cpp
    Neo_refract.cpp
    Neo_refract_dx9_helper.cpp
    Neo_refract_dx9_helper.h
)
