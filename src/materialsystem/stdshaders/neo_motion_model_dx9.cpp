//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example shader modified for models in motions
//
//==================================================================================================

#include "BaseVSShader.h"
#include "convar.h"
#include "neo_motion_model_dx9_helper.h"

DEFINE_FALLBACK_SHADER( Neo_Motion_Model, Neo_Motion_Model_DX9 )
BEGIN_VS_SHADER( Neo_Motion_Model_DX9, "Help for motion model shader" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM(ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "")
		SHADER_PARAM( SPEED, SHADER_PARAM_TYPE_FLOAT, "0.0", "")
	END_SHADER_PARAMS

	void SetupVars( NeoMotionModel_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureFrame = FRAME;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nAlphaTestReference = ALPHATESTREFERENCE;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
		info.m_nSpeed = SPEED;
	}

	SHADER_INIT_PARAMS()
	{
		NeoMotionModel_DX9_Vars_t info;
		SetupVars( info );
		InitParamsNeoMotionModel_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

		SHADER_INIT
	{
		NeoMotionModel_DX9_Vars_t info;
		SetupVars(info);
		InitNeoMotionModel_DX9(this, params, info);
	}

		SHADER_DRAW
	{
		SHADOW_STATE
		{
			//SetInitialShadowState();
			EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE);
			pShaderShadow->EnableDepthWrites(true);
			pShaderShadow->EnableDepthTest(true);
			pShaderShadow->DepthFunc(SHADER_DEPTHFUNC_NEAREROREQUAL);
		}

		DYNAMIC_STATE
		{
			VMatrix mat;
			s_pShaderAPI->GetMatrix(MATERIAL_VIEW, mat.m[0]);
			MatrixTranspose(mat, mat);
			s_pShaderAPI->SetPixelShaderConstant(0, mat.m[2], 3);
			const float speed = min(1.f, params[SPEED]->GetFloatValue() * 0.02f);
			s_pShaderAPI->SetPixelShaderConstant(3, &speed);
		}
		
		NeoMotionModel_DX9_Vars_t info;
		SetupVars( info );
		DrawNeoMotionModel_DX9( this, params, pShaderAPI, pShaderShadow, info, vertexCompression, pContextDataPtr );
	}

END_SHADER

