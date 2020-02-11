#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_noise_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_mv_noise_level("mat_neo_mv_noise_level", "0.75", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f);
ConVar mat_neo_mv_noise_brightness_scale("mat_neo_mv_noise_brightness_scale", "2.0", FCVAR_CHEAT, "");

BEGIN_SHADER_FLAGS(Neo_Noise, "Help for Neo_Noise.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FRAMEBUFFER, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[FRAMEBUFFER]->IsDefined())
	{
		LoadTexture(FRAMEBUFFER);
	}
	else
	{
		Assert(false);
	}
}

bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame /* = true */) const
{
	return true;
}

SHADER_FALLBACK
{
	// Requires DX9 + above
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Assert(0);
		return "Neo_NightVision";
	}
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);

		const int fmt = VERTEX_POSITION;
		const int nTexCoordCount = 1;
		pShaderShadow->VertexShaderVertexFormat(fmt, nTexCoordCount, NULL, 0);

		pShaderShadow->EnableDepthWrites(false);

		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_noise_ps30);
		SET_STATIC_PIXEL_SHADER(neo_noise_ps30);

		// On DX9, get the gamma read and write correct
		if (g_pHardwareConfig->SupportsSRGB())
		{
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, true);
			pShaderShadow->EnableSRGBWrite(true);
		}
		else
		{
			Assert(false);
		}
	}

		DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FRAMEBUFFER);

		const float flRandomSeed = RandomFloat(-4096, 4096);
		const float flNoiseLevel = mat_neo_mv_noise_level.GetFloat();
		const float flBrightnessScale = mat_neo_mv_noise_brightness_scale.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &flRandomSeed);
		pShaderAPI->SetPixelShaderConstant(1, &flNoiseLevel);
		pShaderAPI->SetPixelShaderConstant(2, &flBrightnessScale);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_noise_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_noise_ps30);
	}
	Draw();
}
END_SHADER
