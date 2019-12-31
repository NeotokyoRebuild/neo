#include "BaseVSShader.h"

//#include "SDK_screenspaceeffect_vs20.inc"
//#include "SDK_Bloom_ps20.inc"
//#include "SDK_Bloom_ps20b.inc"

#include "neo_passthrough_vs30.inc"
#include "neo_nightvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_nv_brightness_scale("mat_neo_nv_brightness_scale", "5", FCVAR_CHEAT);
ConVar mat_neo_nv_green_scale("mat_neo_nv_green_scale", "0.1", FCVAR_CHEAT, "Amount of green hue in nightvision.", true, 0.0, false, 0.0);
ConVar mat_neo_nv_bright_flatness("mat_neo_nv_bright_flatness", "3.0", FCVAR_CHEAT);
ConVar mat_neo_nv_dim_flatness("mat_neo_nv_dim_flatness", "4.0", FCVAR_CHEAT);

BEGIN_SHADER_FLAGS(Neo_NightVision, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallHDR0", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FBTEXTURE]->IsDefined())
	{
		LoadTexture(FBTEXTURE);
	}
	else
	{
		Assert(false);
	}

	if (params[BLURTEXTURE]->IsDefined())
	{
		LoadTexture(BLURTEXTURE);
	}
	else
	{
		Assert(false);
	}
}

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

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(false);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		const int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, NULL, 0);

		// Pre-cache shaders
		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_STATIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);
		BindTexture(SHADER_SAMPLER1, BLURTEXTURE, -1);

		const float flBrightnessScale = mat_neo_nv_brightness_scale.GetFloat();
		const float flGreenScale = mat_neo_nv_green_scale.GetFloat();
		const float flBrightFlatness = mat_neo_nv_bright_flatness.GetFloat();
		const float flDimFlatness = mat_neo_nv_dim_flatness.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &flBrightnessScale);
		pShaderAPI->SetPixelShaderConstant(1, &flGreenScale);
		pShaderAPI->SetPixelShaderConstant(2, &flBrightFlatness);
		pShaderAPI->SetPixelShaderConstant(3, &flDimFlatness);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	Draw();
}

END_SHADER