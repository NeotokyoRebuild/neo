#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_motionvision_pass2_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_neo_mv_brightness_multiplier("mat_neo_mv_brightness_multiplier", "0.1", FCVAR_CHEAT | FCVAR_ARCHIVE, "Darken the whole image", true, -1.0f, true, 1.0f);
ConVar mat_neo_mv_x_offset("mat_neo_mv_x_offset", "0", FCVAR_CHEAT | FCVAR_ARCHIVE, "Gradient offset");
ConVar mat_neo_mv_x_multiplier("mat_neo_mv_x_multiplier", "1", FCVAR_CHEAT | FCVAR_ARCHIVE, "Gradient multiplier");

BEGIN_SHADER_FLAGS(Neo_MotionVision_Pass2, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(MOTIONEFFECT, SHADER_PARAM_TYPE_TEXTURE, "_rt_MotionVision", "")
SHADER_PARAM(ORIGINAL, SHADER_PARAM_TYPE_TEXTURE, "_rt_MotionVision_Intermediate2", "")
SHADER_PARAM(MVTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/mvgrad", "")
SHADER_PARAM(NOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/noise", "")
SHADER_PARAM(NOISETRANSFORM, SHADER_PARAM_TYPE_VEC3, "Vector(0, 0, 0)", "")
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

	if (params[MVTEXTURE]->IsDefined())
	{
		LoadTexture(MVTEXTURE);
	}
	else
	{
		Assert(false);
	}

	if (params[NOISETEXTURE]->IsDefined())
	{
		LoadTexture(NOISETEXTURE);
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
		pShaderShadow->EnableTexture(SHADER_SAMPLER2, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);
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
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER2, true);
			pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, true);
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
		BindTexture(SHADER_SAMPLER2, MVTEXTURE);
		BindTexture(SHADER_SAMPLER3, NOISETEXTURE);

		//pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0);

		const float brightness = mat_neo_mv_brightness_multiplier.GetFloat();
		const float* noiseTransformVector = params[NOISETRANSFORM]->GetVecValue();
		const float xOffset = mat_neo_mv_x_offset.GetFloat();
		const float xMultiplier = mat_neo_mv_x_multiplier.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, &brightness);
		pShaderAPI->SetPixelShaderConstant(1, &noiseTransformVector[0]);
		pShaderAPI->SetPixelShaderConstant(2, &noiseTransformVector[1]);
		pShaderAPI->SetPixelShaderConstant(3, &xOffset);
		pShaderAPI->SetPixelShaderConstant(4, &xMultiplier);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
	}
	Draw();
}
END_SHADER
