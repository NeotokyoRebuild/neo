#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_motionvision_pass2_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_mv_brightness_multiplier("mat_neo_mv_brightness_multiplier", "0.5", FCVAR_CHEAT | FCVAR_ARCHIVE, "Darken the whole image", true, -1.0f, true, 1.0f);
ConVar mat_neo_mv_bw_tint_start("mat_neo_mv_bw_tint_start", "0.025", FCVAR_CHEAT | FCVAR_ARCHIVE, "When to start adding tint to the image.", true, -1.0f, true, 1.0f);
ConVar mat_neo_mv_bw_tint_color_r("mat_neo_mv_bw_tint_color_r", "-0.65", FCVAR_CHEAT | FCVAR_ARCHIVE, "Additional normalized Red color tint for the grayscale MV effect.", true, -100.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_color_g("mat_neo_mv_bw_tint_color_g", "-0.8", FCVAR_CHEAT | FCVAR_ARCHIVE, "Additional normalized Green color tint for the grayscale MV effect.", true, -100.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_color_b("mat_neo_mv_bw_tint_color_b", "-0.775", FCVAR_CHEAT | FCVAR_ARCHIVE, "Additional normalized Blue color tint for the grayscale MV effect.", true, -100.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_threshold_r("mat_neo_mv_bw_tint_threshold_r", "0.125", FCVAR_CHEAT | FCVAR_ARCHIVE, "Threshold after which red channel value grows by multiplier.", true, -1.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_threshold_g("mat_neo_mv_bw_tint_threshold_g", "0.425", FCVAR_CHEAT | FCVAR_ARCHIVE, "Threshold after which green channel value grows by multiplier.", true, -1.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_threshold_b("mat_neo_mv_bw_tint_threshold_b", "0.45", FCVAR_CHEAT | FCVAR_ARCHIVE, "Threshold after which blue channel value grows by multiplier.", true, -1.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_exponent("mat_neo_mv_bw_tint_exponent", "2", FCVAR_CHEAT | FCVAR_ARCHIVE, "Exponent", true, -1.0f, true, 100.0f);
ConVar mat_neo_mv_bw_tint_multiplier("mat_neo_mv_bw_tint_multiplier", "10", FCVAR_CHEAT | FCVAR_ARCHIVE, "Multiplier", true, -1.0f, true, 100.0f);

BEGIN_SHADER_FLAGS(Neo_MotionVision_Pass2, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(MOTIONEFFECT, SHADER_PARAM_TYPE_TEXTURE, "_rt_MotionVision", "")
SHADER_PARAM(ORIGINAL, SHADER_PARAM_TYPE_TEXTURE, "_rt_MotionVision_Intermediate2", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[MOTIONEFFECT]->IsDefined())
	{
		LoadTexture(MOTIONEFFECT);
	}
	else
	{
		Assert(false);
	}

	if (params[ORIGINAL]->IsDefined())
	{
		LoadTexture(ORIGINAL);
	}
	else
	{
		Assert(false);
	}
}

#if(0)
bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame /* = true */) const
{
	return true;
}
#endif

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
		//SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		//pShaderShadow->BlendFunc(SHADER_BLEND_ZERO, SHADER_BLEND_ONE);

		const int fmt = VERTEX_POSITION;
		const int nTexCoordCount = 1;
		pShaderShadow->VertexShaderVertexFormat(fmt, nTexCoordCount, NULL, 0);

		pShaderShadow->EnableDepthWrites(false);
		//EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE);
		//pShaderShadow->EnableBlending(true);
		//pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );
		//pShaderShadow->EnableBlendingSeparateAlpha(true);
		//pShaderShadow->BlendFuncSeparateAlpha(SHADER_BLEND_ZERO, SHADER_BLEND_SRC_ALPHA);

		//SetInitialShadowState();

		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
		SET_STATIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);

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
		//pShaderAPI->SetDefaultState();

		BindTexture(SHADER_SAMPLER0, MOTIONEFFECT);
		BindTexture(SHADER_SAMPLER1, ORIGINAL);

		//pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0);

		const float brightness = mat_neo_mv_brightness_multiplier.GetFloat();
		const float start = mat_neo_mv_bw_tint_start.GetFloat();
		const float r = mat_neo_mv_bw_tint_color_r.GetFloat();
		const float g = mat_neo_mv_bw_tint_color_g.GetFloat();
		const float b = mat_neo_mv_bw_tint_color_b.GetFloat();
		const float rT = mat_neo_mv_bw_tint_threshold_r.GetFloat();
		const float gT = mat_neo_mv_bw_tint_threshold_g.GetFloat();
		const float bT = mat_neo_mv_bw_tint_threshold_b.GetFloat();
		const float exponent = mat_neo_mv_bw_tint_exponent.GetFloat();
		const float multiplier = mat_neo_mv_bw_tint_multiplier.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &brightness);
		pShaderAPI->SetPixelShaderConstant(1, &start);
		pShaderAPI->SetPixelShaderConstant(2, &r);
		pShaderAPI->SetPixelShaderConstant(3, &g);
		pShaderAPI->SetPixelShaderConstant(4, &b);
		pShaderAPI->SetPixelShaderConstant(5, &rT);
		pShaderAPI->SetPixelShaderConstant(6, &gT);
		pShaderAPI->SetPixelShaderConstant(7, &bT);
		pShaderAPI->SetPixelShaderConstant(8, &exponent);
		pShaderAPI->SetPixelShaderConstant(9, &multiplier);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
	}
	Draw();
}
END_SHADER
