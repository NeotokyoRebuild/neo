#include "BaseVSShader.h"

//#include "SDK_screenspaceeffect_vs20.inc"
//#include "SDK_Bloom_ps20.inc"
//#include "SDK_Bloom_ps20b.inc"

#include "neo_passthrough_vs30.inc"
#include "neo_nightvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_nv_brightness("mat_neo_nv_brightness", "0.2", FCVAR_CHEAT);
ConVar mat_neo_nv_green_scale("mat_neo_nv_green_scale", "0.15", FCVAR_CHEAT, "Amount of green hue in nightvision.", true, 0.0, false, 0.0);
ConVar mat_neo_nv_contrast("mat_neo_nv_contrast", "1.75", FCVAR_CHEAT);
ConVar mat_neo_nv_luminance("mat_neo_nv_luminance", "0.2", FCVAR_CHEAT);
// These are not the actual screen gamma values, but rather a range from which to emulate the nv gamma adjust.
ConVar mat_neo_nv_startgamma("mat_neo_nv_startgamma", "2.2", FCVAR_CHEAT);
ConVar mat_neo_nv_targetgamma("mat_neo_nv_targetgamma", "2.4", FCVAR_CHEAT);

BEGIN_SHADER_FLAGS(Neo_NightVision, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
//SHADER_PARAM(BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallHDR0", "")
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

	//if (params[BLURTEXTURE]->IsDefined())
	//{
	//	LoadTexture(BLURTEXTURE);
	//}
	//else
	//{
	//	Assert(false);
	//}
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
		//pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);

		// Pre-cache shaders
		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_STATIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE, -1);
		//BindTexture(SHADER_SAMPLER1, BLURTEXTURE, -1);

		const float flBrightnessScale = mat_neo_nv_brightness.GetFloat();
		const float flGreenScale = mat_neo_nv_green_scale.GetFloat();
		const float flContrast = mat_neo_nv_contrast.GetFloat();
		const float flLuminance = mat_neo_nv_luminance.GetFloat();
		const float flStartGamma = mat_neo_nv_startgamma.GetFloat();
		const float flTargetGamma = mat_neo_nv_targetgamma.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &flBrightnessScale);
		pShaderAPI->SetPixelShaderConstant(1, &flGreenScale);
		pShaderAPI->SetPixelShaderConstant(2, &flContrast);
		pShaderAPI->SetPixelShaderConstant(3, &flLuminance);
		pShaderAPI->SetPixelShaderConstant(4, &flStartGamma);
		pShaderAPI->SetPixelShaderConstant(5, &flTargetGamma);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	Draw();
}

END_SHADER