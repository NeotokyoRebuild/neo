//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example shader modified for models in thermals
//
//==================================================================================================

#include "BaseVSShader.h"
#include "convar.h"
#include "neo_thermal_model_dx9_helper.h"

DEFINE_FALLBACK_SHADER( Neo_Thermal_Model, Neo_Thermal_Model_DX9 )
BEGIN_VS_SHADER( Neo_Thermal_Model_DX9, "Help for thermal model shader" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )	
	END_SHADER_PARAMS

	void SetupVars( NeoThermalModel_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nBaseTextureFrame = FRAME;
		info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;
		info.m_nAlphaTestReference = ALPHATESTREFERENCE;
		info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;
		info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;
	}

	SHADER_INIT_PARAMS()
	{
		NeoThermalModel_DX9_Vars_t info;
		SetupVars( info );
		InitParamsNeoThermalModel_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

		SHADER_INIT
	{
		NeoThermalModel_DX9_Vars_t info;
		SetupVars(info);
		InitNeoThermalModel_DX9(this, params, info);
	}

		SHADER_DRAW
	{
		if (pShaderAPI)
		{
			VMatrix mat, transpose;
			s_pShaderAPI->GetMatrix(MATERIAL_VIEW, mat.m[0]);
			MatrixTranspose(mat, transpose);
			s_pShaderAPI->SetPixelShaderConstant(2, transpose.m[2], 3);
		}

		NeoThermalModel_DX9_Vars_t info;
		SetupVars( info );
		DrawNeoThermalModel_DX9( this, params, pShaderAPI, pShaderShadow, info, vertexCompression, pContextDataPtr );
	}

END_SHADER

