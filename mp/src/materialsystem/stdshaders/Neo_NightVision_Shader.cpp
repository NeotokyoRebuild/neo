#include "BaseVSShader.h"

#include "neo_nightvision_vs30.inc"
#include "neo_nightvision_ps30.inc"

BEGIN_SHADER(Neo_NightVision, "Help for my shader.")

BEGIN_SHADER_PARAMS
END_SHADER_PARAMS

SHADER_INIT
{
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
		return "Wireframe";
	}
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		const int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

		pShaderShadow->EnableDepthWrites(false);

		DECLARE_STATIC_VERTEX_SHADER(neo_nightvision_vs30);
		SET_STATIC_VERTEX_SHADER(neo_nightvision_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_STATIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	DYNAMIC_STATE
	{
		DECLARE_DYNAMIC_VERTEX_SHADER(neo_nightvision_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_nightvision_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_nightvision_ps30);
	}

	Draw();
}

END_SHADER