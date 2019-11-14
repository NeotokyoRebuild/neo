#include "BaseVSShader.h"

#include "neo_ssao_vs30.inc"
#include "neo_ssao_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
//#include "tier0/memdbgon.h"

ConVar mat_neo_ssao_samples("mat_neo_ssao_samples", "16", FCVAR_CHEAT, "", true, 8.0f, true, 32.0f);
ConVar mat_neo_ssao_contrast("mat_neo_ssao_contrast", "0.55", FCVAR_CHEAT, "", true, 0.0f, false, 0.0f);
ConVar mat_neo_ssao_radius("mat_neo_ssao_radius", "8", FCVAR_CHEAT, "", true, 1.0f, false, 128.0f);
ConVar mat_neo_ssao_bias("mat_neo_ssao_bias", "0.02", FCVAR_CHEAT, "", true, 0.0f, false, 10.0f);
ConVar mat_neo_ssao_bias_offset("mat_neo_ssao_bias_offset", "0.05", FCVAR_CHEAT, "", true, 0.05f, true, 0.05f);
ConVar mat_neo_ssao_illuminfluence("mat_neo_ssao_illuminfluence", "0", FCVAR_CHEAT, "", true, 0.0f, false, 10.0f);
ConVar mat_neo_ssao_zfar("mat_neo_ssao_zfar", "8", FCVAR_CHEAT, "", true, 0.0f, true, 512.0f);
ConVar mat_neo_ssao_znear("mat_neo_ssao_znear", "1", FCVAR_CHEAT, "", true, 0.0f, true, 512.0f);

BEGIN_SHADER(Neo_SSAO, "Help for my shader.")

BEGIN_SHADER_PARAMS
	SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Full frame buffer")
END_SHADER_PARAMS

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture(BASETEXTURE);
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

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);

		DECLARE_STATIC_VERTEX_SHADER(neo_ssao_vs30);
		SET_STATIC_VERTEX_SHADER(neo_ssao_vs30);

		DECLARE_STATIC_PIXEL_SHADER(neo_ssao_ps30);
		SET_STATIC_PIXEL_SHADER(neo_ssao_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE);

		ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();

		const int width = src_texture->GetActualWidth();
		const int height = src_texture->GetActualHeight();

		const float g_TexelSize[2] = { 1.0f / float(width), 1.0f / float(height) };
		const float samples = mat_neo_ssao_samples.GetInt();
		const float radius = mat_neo_ssao_radius.GetFloat();
		const float bias = mat_neo_ssao_bias.GetFloat();
		const float illuminf = mat_neo_ssao_illuminfluence.GetFloat();
		const float contrast = mat_neo_ssao_contrast.GetFloat();
		const float znear = mat_neo_ssao_znear.GetFloat();
		const float zfar = mat_neo_ssao_zfar.GetFloat();
		const float biasoffset = mat_neo_ssao_bias_offset.GetFloat();

		pShaderAPI->SetPixelShaderConstant(0, g_TexelSize);
		pShaderAPI->SetPixelShaderConstant(1, &samples);
		pShaderAPI->SetPixelShaderConstant(2, &radius);
		pShaderAPI->SetPixelShaderConstant(3, &bias);
		pShaderAPI->SetPixelShaderConstant(4, &illuminf);
		pShaderAPI->SetPixelShaderConstant(5, &contrast);
		pShaderAPI->SetPixelShaderConstant(6, &znear);
		pShaderAPI->SetPixelShaderConstant(7, &zfar);
		pShaderAPI->SetPixelShaderConstant(8, &biasoffset);

		DECLARE_DYNAMIC_VERTEX_SHADER(neo_ssao_vs30);
		SET_DYNAMIC_VERTEX_SHADER(neo_ssao_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(neo_ssao_ps30);
		SET_DYNAMIC_PIXEL_SHADER(neo_ssao_ps30);
	}

	Draw();
}
END_SHADER
