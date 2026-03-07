#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_thermalvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER_FLAGS(Neo_ThermalVision, "Help for the thermalvision shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(TVTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/tvgrad2", "")
SHADER_PARAM(NOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/noise", "")
SHADER_PARAM(NOISETRANSFORM, SHADER_PARAM_TYPE_VEC2, "[0 0]", "")
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

	if (params[TVTEXTURE]->IsDefined())
	{
		LoadTexture(TVTEXTURE);
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
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER2, true);
		
		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, NULL, 0);
		
		pShaderShadow->EnableDepthWrites(false);

		DECLARE_STATIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_STATIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_thermalvision_ps30);
		SET_STATIC_PIXEL_SHADER(neo_thermalvision_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, FBTEXTURE);
		BindTexture(SHADER_SAMPLER1, TVTEXTURE);
		BindTexture(SHADER_SAMPLER2, NOISETEXTURE);
		
		//s_pShaderAPI->SetPixelShaderConstant(0, NOISETRANSFORM);
		float vNoiseTransform[2] = {0, 0};
		params[NOISETRANSFORM]->GetVecValue(vNoiseTransform, 2);
		s_pShaderAPI->SetPixelShaderConstant(0, vNoiseTransform);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
	}

	Draw();
}
END_SHADER
