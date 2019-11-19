#include "BaseVSShader.h"

#include "neo_motionvision_pass2_vs30.inc"
#include "neo_motionvision_pass2_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

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

		DECLARE_STATIC_VERTEX_SHADER(neo_motionvision_pass2_vs30);
		SET_STATIC_VERTEX_SHADER(neo_motionvision_pass2_vs30);

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

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_motionvision_pass2_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_motionvision_pass2_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_motionvision_pass2_ps30);
	}
	Draw();
}
END_SHADER
