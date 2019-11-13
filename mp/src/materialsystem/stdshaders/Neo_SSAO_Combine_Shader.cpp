#include "BaseVSShader.h"

//==========================================
// Set your shader name here
#define THIS_SHADER_NAME neo_ssao_combine
//==========================================
// Set your DX shader model ver number here
#define THIS_SHADER_MODEL_NUM 30
//==========================================

// A bunch of macro boilerplate to avoid repeating or mixing up shader names.
#define VS_MODEL_NUM PPCAT(_vs, THIS_SHADER_MODEL_NUM)
#define PS_MODEL_NUM PPCAT(_ps, THIS_SHADER_MODEL_NUM)
#define THIS_SHADER_VS PPCAT(THIS_SHADER_NAME, VS_MODEL_NUM)
#define THIS_SHADER_VS_NX PPCAT_NX(THIS_SHADER_NAME, VS_MODEL_NUM)
#define THIS_SHADER_PS PPCAT(THIS_SHADER_NAME, PS_MODEL_NUM)

// Compiled shader includes
#include "neo_ssao_combine_ps30.inc"
#include "neo_ssao_combine_vs30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS(THIS_SHADER_NAME, "Help for "STRINGIZE(THIS_SHADER_NAME), SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Framebuffer")
SHADER_PARAM(SSAOTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SSAO", "SSAO")
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);
}

SHADER_FALLBACK
{
	Assert(false);
	return "Wireframe";
}

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture(BASETEXTURE);
	}

	if (params[SSAOTEXTURE]->IsDefined())
	{
		LoadTexture(SSAOTEXTURE);
	}
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(false);

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, 0, 0);

		DECLARE_STATIC_VERTEX_SHADER_X(THIS_SHADER_VS);
		SET_STATIC_VERTEX_SHADER_X(THIS_SHADER_VS);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);

		DECLARE_STATIC_PIXEL_SHADER_X(THIS_SHADER_PS);
		SET_STATIC_PIXEL_SHADER_X(THIS_SHADER_PS);

		// Render targets are pegged as sRGB on POSIX, so just force these reads and writes
#ifdef LINUX
		const bool bForceSRGBReadAndWrite = g_pHardwareConfig->CanDoSRGBReadFromRTs();
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, bForceSRGBReadAndWrite);
		pShaderShadow->EnableSRGBWrite(bForceSRGBReadAndWrite);
#elif defined(_WIN32)
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, false);
		pShaderShadow->EnableSRGBWrite(false);
#else // OS X
		const bool bForceSRGBReadAndWrite = IsOSX() && g_pHardwareConfig->CanDoSRGBReadFromRTs();
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, bForceSRGBReadAndWrite);
		pShaderShadow->EnableSRGBWrite(bForceSRGBReadAndWrite);
#endif
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE, -1);
		BindTexture(SHADER_SAMPLER1, SSAOTEXTURE, -1);

		DECLARE_DYNAMIC_VERTEX_SHADER_X(THIS_SHADER_VS);
		SET_DYNAMIC_VERTEX_SHADER_X(THIS_SHADER_VS);

		DECLARE_DYNAMIC_PIXEL_SHADER_X(THIS_SHADER_PS);
		SET_DYNAMIC_PIXEL_SHADER_X(THIS_SHADER_PS);
	}
	Draw();
}
END_SHADER
