#include "BaseVSShader.h"

//==========================================
// Set your shader name here
#define THIS_SHADER_NAME neo_ssao
//==========================================
// Set your DX shader model ver number here
#define THIS_SHADER_MODEL_NUM 30
//==========================================

// A bunch of macro boilerplate to avoid repeating or mixing up shader names.
#define VS_MODEL_NUM PPCAT(_vs, THIS_SHADER_MODEL_NUM)
#define PS_MODEL_NUM PPCAT(_ps, THIS_SHADER_MODEL_NUM)
#define THIS_SHADER_VS PPCAT(THIS_SHADER_NAME, VS_MODEL_NUM)
#define THIS_SHADER_VS_NX PPCAT_NX(THIS_SHADER_NAME, VS_MODEL_NUM)
#define THIS_SHADER_PS PPCAT(THIS_SHADER_NAME, PS_MODEL_NUM)
#define SHADER_BINARY_ETX .inc
#define THIS_SHADER_VS_BINARY STRINGIZE(PPCAT(THIS_SHADER_VS, SHADER_BINARY_ETX))
#define THIS_SHADER_PS_BINARY STRINGIZE(PPCAT(THIS_SHADER_PS, SHADER_BINARY_ETX))

// Compiled shader includes
#include THIS_SHADER_VS_BINARY
#include THIS_SHADER_PS_BINARY

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// SHADER PARAMS DEFINED IN SHADER FXC CODE
ConVar mat_neo_ssao_samples("mat_neo_ssao_samples", "8");
ConVar mat_neo_ssao_contrast("mat_neo_ssao_contrast", "2.0");
ConVar mat_neo_ssao_radius("mat_neo_ssao_radius", "16");
ConVar mat_neo_ssao_bias("mat_neo_ssao_bias", "0.02");
ConVar mat_neo_ssao_bias_offset("mat_neo_ssao_bias_offset", "0.05");
ConVar mat_neo_ssao_illuminfluence("mat_neo_ssao_illuminfluence", "5.0");
ConVar mat_neo_ssao_zfar("mat_neo_ssao_zfar", "8.0");
ConVar mat_neo_ssao_znear("mat_neo_ssao_znear", "1.0");

BEGIN_VS_SHADER_FLAGS(THIS_SHADER_NAME, "Help for "STRINGIZE(THIS_SHADER_NAME), SHADER_NOT_EDITABLE)
BEGIN_SHADER_PARAMS
SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Framebuffer")
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);
}

SHADER_FALLBACK
{
	return 0;
}

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture(BASETEXTURE);
	}
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, 0, 0);

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);

		// Render targets are pegged as sRGB on POSIX, so just force these reads and writes
		bool bForceSRGBReadAndWrite = IsOSX() && g_pHardwareConfig->CanDoSRGBReadFromRTs();
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, bForceSRGBReadAndWrite);
		pShaderShadow->EnableSRGBWrite(bForceSRGBReadAndWrite);

		DECLARE_STATIC_VERTEX_SHADER_X(THIS_SHADER_VS);
		SET_STATIC_VERTEX_SHADER_X(THIS_SHADER_VS);

		DECLARE_STATIC_PIXEL_SHADER_X(THIS_SHADER_PS);
		SET_STATIC_PIXEL_SHADER_X(THIS_SHADER_PS);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE, -1);

		ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();

		int width = src_texture->GetActualWidth();
		int height = src_texture->GetActualHeight();

		float g_TexelSize[2] = { 1.0f / float(width), 1.0f / float(height) };

		pShaderAPI->SetPixelShaderConstant(0, g_TexelSize);

		DECLARE_DYNAMIC_VERTEX_SHADER_X(THIS_SHADER_VS);
		SET_DYNAMIC_VERTEX_SHADER_X(THIS_SHADER_VS);

		DECLARE_DYNAMIC_PIXEL_SHADER_X(THIS_SHADER_PS);
		SET_DYNAMIC_PIXEL_SHADER_X(THIS_SHADER_PS);

		float samples = mat_neo_ssao_samples.GetInt();
		float contrast = mat_neo_ssao_contrast.GetFloat();
		float radius = mat_neo_ssao_radius.GetFloat();
		float bias = mat_neo_ssao_bias.GetFloat();
		float biasoffset = mat_neo_ssao_bias_offset.GetFloat();
		float illuminf = mat_neo_ssao_illuminfluence.GetFloat();
		float zfar = mat_neo_ssao_zfar.GetFloat();
		float znear = mat_neo_ssao_znear.GetFloat();

		pShaderAPI->SetPixelShaderConstant(1, &samples);
		pShaderAPI->SetPixelShaderConstant(2, &radius);
		pShaderAPI->SetPixelShaderConstant(3, &bias);
		pShaderAPI->SetPixelShaderConstant(4, &illuminf);
		pShaderAPI->SetPixelShaderConstant(5, &contrast);
		pShaderAPI->SetPixelShaderConstant(6, &znear);
		pShaderAPI->SetPixelShaderConstant(7, &zfar);
		pShaderAPI->SetPixelShaderConstant(8, &biasoffset);
	}
	Draw();
}
END_SHADER
