#include "BaseVSShader.h"

// Compiled shader includes
#include "neo_passthrough_vs30.inc"
#include "neo_gaussianblur_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS(Neo_GaussianBlur, "Help for Neo_GaussianBlur", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Render target")
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

		DECLARE_STATIC_VERTEX_SHADER_X(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER_X(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER_X(neo_gaussianblur_ps30);
		SET_STATIC_PIXEL_SHADER_X(neo_gaussianblur_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE, -1);

		ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();

		const int width = src_texture->GetActualWidth();
		const int height = src_texture->GetActualHeight();

		const float fTexelSize[2] = { 1.0f / (float)width, 1.0f / (float)height };

		pShaderAPI->SetPixelShaderConstant(0, fTexelSize);

		DECLARE_DYNAMIC_VERTEX_SHADER_X(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER_X(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER_X(neo_gaussianblur_ps30);
		SET_DYNAMIC_PIXEL_SHADER_X(neo_gaussianblur_ps30);
	}
	Draw();
}
END_SHADER
