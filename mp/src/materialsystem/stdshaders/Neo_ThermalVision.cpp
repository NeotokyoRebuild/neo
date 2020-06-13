#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_thermalvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_tv_brightness_scale("mat_neo_tv_brightness_scale", "2", FCVAR_CHEAT);
ConVar mat_neo_tv_xoffset("mat_neo_tv_xoffset", "0.4", FCVAR_CHEAT);

BEGIN_SHADER_FLAGS(Neo_ThermalVision, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
#if(1)
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_ThermalVision", "")
//SHADER_PARAM(BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallHDR0", "")
SHADER_PARAM(TVTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/tvgrad2", "")
//SHADER_PARAM(NOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/noise", "")
#endif
END_SHADER_PARAMS

SHADER_INIT
{
#if(1)
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

	if (params[TVTEXTURE]->IsDefined())
	{
		LoadTexture(TVTEXTURE);
	}
	else
	{
		Assert(false);
	}

	//if (params[NOISETEXTURE]->IsDefined())
	//{
	//	LoadTexture(NOISETEXTURE);
	//}
	//else
	//{
	//	Assert(false);
	//}
#endif
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
#if(1)
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		//pShaderShadow->EnableTexture(SHADER_SAMPLER2, true);
		//pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);
#endif

		const int fmt = VERTEX_POSITION;
		const int nTexCoordCount = 1;
		pShaderShadow->VertexShaderVertexFormat(fmt, nTexCoordCount, NULL, 0);

		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_thermalvision_ps30);
		SET_STATIC_PIXEL_SHADER(neo_thermalvision_ps30);

		// On DX9, get the gamma read and write correct
		if (g_pHardwareConfig->SupportsSRGB())
		{
#if(1)
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, true);
			//pShaderShadow->EnableSRGBRead(SHADER_SAMPLER2, true);
			//pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, true);
#endif
			pShaderShadow->EnableSRGBWrite(true);
		}
		else
		{
			Assert(false);
		}
	}

	DYNAMIC_STATE
	{
#if(1)
		BindTexture(SHADER_SAMPLER0, FBTEXTURE);
		//BindTexture(SHADER_SAMPLER1, BLURTEXTURE);
		BindTexture(SHADER_SAMPLER1, TVTEXTURE);
		//BindTexture(SHADER_SAMPLER3, NOISETEXTURE);
#endif

		const float flBrightnessScale = mat_neo_tv_brightness_scale.GetFloat();
		const float flXOffset = mat_neo_tv_xoffset.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &flBrightnessScale);
		pShaderAPI->SetPixelShaderConstant(1, &flXOffset);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
	}

	Draw();
}
END_SHADER
