#include "BaseVSShader.h"

#include "neo_passthrough_vs30.inc"
#include "neo_thermalvision_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Sic: this name is typo'd to match the materials/dev/thermalvision vmt naming.
BEGIN_SHADER_FLAGS(neo_thermalvison_tv, "Help for my shader.", SHADER_NOT_EDITABLE)

BEGIN_SHADER_PARAMS
SHADER_PARAM(FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
SHADER_PARAM(BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallHDR0", "")
SHADER_PARAM(TVTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/tvgrad2", "")
SHADER_PARAM(NOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "dev/noise", "")
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
		pShaderShadow->EnableTexture(SHADER_SAMPLER3, true);

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

		DYNAMIC_STATE
		{
			BindTexture(SHADER_SAMPLER0, FBTEXTURE);
			BindTexture(SHADER_SAMPLER1, BLURTEXTURE);
			BindTexture(SHADER_SAMPLER2, TVTEXTURE);
			BindTexture(SHADER_SAMPLER3, NOISETEXTURE);

			DECLARE_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);
			SET_DYNAMIC_VERTEX_SHADER(neo_passthrough_vs30);

			DECLARE_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
			SET_DYNAMIC_PIXEL_SHADER(neo_thermalvision_ps30);
		}

		Draw();
	}
}
END_SHADER
