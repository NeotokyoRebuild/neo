#include "BaseVSShader.h"

//#include "SDK_screenspaceeffect_vs20.inc"
//#include "SDK_Bloom_ps20.inc"
//#include "SDK_Bloom_ps20b.inc"

#include "neo_passthrough_vs30.inc"
#include "neo_nightvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_nv_screentint_red("mat_neo_nv_screentint_red", "0.08", FCVAR_CHEAT);
ConVar mat_neo_nv_screentint_green("mat_neo_nv_screentint_green", "0.09", FCVAR_CHEAT);
ConVar mat_neo_nv_screentint_blue("mat_neo_nv_screentint_blue", "0.08", FCVAR_CHEAT);
ConVar mat_neo_nv_luminosity_mp("mat_neo_nv_luminosity_mp", "0.01", FCVAR_CHEAT);
ConVar mat_neo_nv_luminosity_intensity("mat_neo_nv_luminosity_intensity", "0.25", FCVAR_CHEAT);
ConVar mat_neo_nv_nightvision_intensity("mat_neo_nv_nightvision_intensity", "120", FCVAR_CHEAT);

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

		const float flScreenTintRed = mat_neo_nv_screentint_red.GetFloat();
		const float flScreenTintGreen = mat_neo_nv_screentint_green.GetFloat();
		const float flScreenTintBlue = mat_neo_nv_screentint_blue.GetFloat();
		const float flLuminosityMP = mat_neo_nv_luminosity_mp.GetFloat();
		const float flLuminosityIntensity = mat_neo_nv_luminosity_intensity.GetFloat();
		const float flNightvisionIntensity = mat_neo_nv_nightvision_intensity.GetFloat();

		pShaderAPI->SetPixelShaderConstant(1, &flScreenTintRed);
		pShaderAPI->SetPixelShaderConstant(2, &flScreenTintGreen);
		pShaderAPI->SetPixelShaderConstant(3, &flScreenTintBlue);
		pShaderAPI->SetPixelShaderConstant(4, &flLuminosityMP);
		pShaderAPI->SetPixelShaderConstant(5, &flLuminosityIntensity);
		pShaderAPI->SetPixelShaderConstant(6, &flNightvisionIntensity);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	Draw();
}

END_SHADER