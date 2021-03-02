#include "BaseVSShader.h"

// Compiled shader includes
#include "neo_passthrough_vs30.inc"
#include "neo_colorblind_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_colorblind_shift_rgb("mat_neo_colorblind_shift_rgb", "0", FCVAR_USERINFO, "Reorder RGB color values.\n\t0: RGB (regular order)\n\t1: GBR\n\t2: BRG\n\t3: BGR\n\t4: GRB\n\t5: RBG", true, 0.0f, true, 5.0f);
ConVar mat_neo_colorblind_deuteranopia_mode("mat_neo_colorblind_deuteranopia_mode", "0", FCVAR_USERINFO, "Enable deuteranopia adjustment. 0: disabled, 1: simulate deuteranopia colors, 2: apply color correction, 3: simulate color corrected deuteranopia colors", true, 0.0f, true, 3.0f);
ConVar mat_neo_colorblind_deuteranopia_brightness("mat_neo_colorblind_deuteranopia_brightness", "1.2", FCVAR_USERINFO, "Post-effect brightness scaler", true, 0.0f, true, 2.2f);
ConVar mat_neo_colorblind_deuteranopia_saturation("mat_neo_colorblind_deuteranopia_saturation", "1.0", FCVAR_CHEAT, "Post-effect saturation adjust", true, 0.0f, true, 2.0f);
ConVar mat_neo_colorblind_deuteranopia_saturation_b("mat_neo_colorblind_deuteranopia_saturation_b", "1.0", FCVAR_USERINFO, "Post-effect saturation adjust exclusively for the blue channel", true, 0.0f, true, 2.0f);

BEGIN_VS_SHADER_FLAGS(Neo_ColorBlind, "Help for Neo_ColorBlind", SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_ColorBlind", "Render target")
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

		DECLARE_STATIC_PIXEL_SHADER_X(neo_colorblind_ps30);
		SET_STATIC_PIXEL_SHADER_X(neo_colorblind_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE, -1);

		const float mode_reorder_rgb_mode = mat_neo_colorblind_shift_rgb.GetFloat();
		const float mode_deuteranopia = mat_neo_colorblind_deuteranopia_mode.GetFloat();
		const float deuteranopia_brightness = mat_neo_colorblind_deuteranopia_brightness.GetFloat();
		const float deuteranopia_saturation = mat_neo_colorblind_deuteranopia_saturation.GetFloat();
		const float deuteranopia_saturation_b = mat_neo_colorblind_deuteranopia_saturation_b.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &mode_reorder_rgb_mode);
		pShaderAPI->SetPixelShaderConstant(1, &mode_deuteranopia);
		pShaderAPI->SetPixelShaderConstant(2, &deuteranopia_brightness);
		pShaderAPI->SetPixelShaderConstant(3, &deuteranopia_saturation);
		pShaderAPI->SetPixelShaderConstant(4, &deuteranopia_saturation_b);

		DECLARE_DYNAMIC_VERTEX_SHADER_X(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER_X(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER_X(neo_colorblind_ps30);
		SET_DYNAMIC_PIXEL_SHADER_X(neo_colorblind_ps30);
	}
	Draw();
}
END_SHADER