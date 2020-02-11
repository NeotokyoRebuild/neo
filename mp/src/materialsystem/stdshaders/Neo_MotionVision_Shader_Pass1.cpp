#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_motionvision_pass1_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_mv_sensitivity("mat_neo_mv_sensitivity", "0.02", FCVAR_CHEAT, "How fast movement gets picked up by motion vision.", true, 0.0f, true, 1.0f);
ConVar mat_neo_mv_color_r("mat_neo_mv_color_r", "0.5", FCVAR_CHEAT, "Normalized RBGA Red color.", true, 0.0f, true, 1.0f);
ConVar mat_neo_mv_color_g("mat_neo_mv_color_g", "0.33", FCVAR_CHEAT, "Normalized RBGA Green color.", true, 0.0f, true, 1.0f);
ConVar mat_neo_mv_color_b("mat_neo_mv_color_b", "0.17", FCVAR_CHEAT, "Normalized RBGA Blue color.", true, 0.0f, true, 1.0f);
ConVar mat_neo_mv_color_a("mat_neo_mv_color_a", "0.165", FCVAR_CHEAT, "Normalized RBGA Alpha color.", true, 0.0f, true, 1.0f);
ConVar mat_neo_mv_speed_modifier("mat_neo_mv_max_speed_modifier", "0.1", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f);

static ConVar neo_this_client_speed("neo_this_client_speed", "0", FCVAR_SPONLY, "", true, 0.0f, true, 1.0f);

BEGIN_SHADER_FLAGS(Neo_MotionVision_Pass1, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(MOTIONORIGIN, SHADER_PARAM_TYPE_TEXTURE, "_rt_MotionVision", "")
SHADER_PARAM(INTERMEDIATE, SHADER_PARAM_TYPE_TEXTURE, "_rt_MotionVision_Buffer1", "")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[MOTIONORIGIN]->IsDefined())
	{
		LoadTexture(MOTIONORIGIN);
	}
	else
	{
		Assert(false);
	}

	if (params[INTERMEDIATE]->IsDefined())
	{
		LoadTexture(INTERMEDIATE);
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
		return "Wireframe";
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

		DECLARE_STATIC_PIXEL_SHADER(neo_motionvision_pass1_ps30);
		SET_STATIC_PIXEL_SHADER(neo_motionvision_pass1_ps30);

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

		BindTexture(SHADER_SAMPLER0, MOTIONORIGIN);
		BindTexture(SHADER_SAMPLER1, INTERMEDIATE);

		//pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0);

		const float flMvSensitivity = mat_neo_mv_sensitivity.GetFloat();
		const float speed = neo_this_client_speed.GetFloat();

		const float r = mat_neo_mv_color_r.GetFloat();
		const float g = mat_neo_mv_color_g.GetFloat();
		const float b = mat_neo_mv_color_b.GetFloat();
		const float a = mat_neo_mv_color_a.GetFloat();

		const float maxSpeedMod = mat_neo_mv_speed_modifier.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &flMvSensitivity);
		pShaderAPI->SetPixelShaderConstant(1, &speed);
		pShaderAPI->SetPixelShaderConstant(2, &r);
		pShaderAPI->SetPixelShaderConstant(3, &g);
		pShaderAPI->SetPixelShaderConstant(4, &b);
		pShaderAPI->SetPixelShaderConstant(5, &a);
		pShaderAPI->SetPixelShaderConstant(6, &maxSpeedMod);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass1_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass1_ps30);
	}
	Draw();
}
END_SHADER
