#include "BaseVSShader.h"

//==========================================
// Set your shader name here
#define THIS_SHADER_NAME neo_gaussianblur
//==========================================
// Set your DX shader model ver number here
#define THIS_SHADER_MODEL_NUM 30
//==========================================

// A bunch of macro boilerplate to avoid repeating or mixing up shader names.
#define VS_MODEL_NUM PPCAT(_vs, THIS_SHADER_MODEL_NUM)
#define PS_MODEL_NUM PPCAT(_ps, THIS_SHADER_MODEL_NUM)
#define THIS_SHADER_VS PPCAT(THIS_SHADER_NAME, VS_MODEL_NUM)
#define THIS_SHADER_PS PPCAT(THIS_SHADER_NAME, PS_MODEL_NUM)

// Compiled shader includes
#include "neo_gaussianblur_ps30.inc"
#include "neo_gaussianblur_vs30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS(THIS_SHADER_NAME, "Help for "STRINGIZE(THIS_SHADER_NAME), SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Render Target")
END_SHADER_PARAMS

SHADER_FALLBACK
{
	// Requires DX9 + above
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Assert(0);
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture(BASETEXTURE);
	}
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableAlphaWrites(true);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, 0, 0);

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

		DECLARE_STATIC_VERTEX_SHADER_X(THIS_SHADER_VS);
		SET_STATIC_VERTEX_SHADER_X(THIS_SHADER_VS);

		DECLARE_STATIC_PIXEL_SHADER_X(THIS_SHADER_PS);
		SET_STATIC_PIXEL_SHADER_X(THIS_SHADER_PS);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE, -1);

		ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();

		int width = src_texture->GetActualWidth();
		int height = src_texture->GetActualHeight();

		float dX = 1.0f / width;
		float dY = 1.0f / height;

		float fTexelSize[2] = { dX, dY };

		pShaderAPI->SetPixelShaderConstant(0, fTexelSize);

		DECLARE_DYNAMIC_VERTEX_SHADER_X(THIS_SHADER_VS);
		SET_DYNAMIC_VERTEX_SHADER_X(THIS_SHADER_VS);

		DECLARE_DYNAMIC_PIXEL_SHADER_X(THIS_SHADER_PS);
		SET_DYNAMIC_PIXEL_SHADER_X(THIS_SHADER_PS);
	}
	Draw();
}
END_SHADER
