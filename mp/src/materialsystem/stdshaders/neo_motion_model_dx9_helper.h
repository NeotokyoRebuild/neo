//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example shader modified for models in motions
//
//==================================================================================================

#ifndef NEO_THERMAL_MODEL_DX9_HELPER_H
#define NEO_THERMAL_MODEL_DX9_HELPER_H

#include <string.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct NeoMotionModel_DX9_Vars_t
{
	NeoMotionModel_DX9_Vars_t() { memset( this, 0xFF, sizeof(*this) ); }

	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nAlphaTestReference;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
	int m_nSpeed;
};

void InitParamsNeoMotionModel_DX9( CBaseVSShader *pShader, IMaterialVar** params,
						 const char *pMaterialName, NeoMotionModel_DX9_Vars_t &info );

void InitNeoMotionModel_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
				   NeoMotionModel_DX9_Vars_t &info );

void DrawNeoMotionModel_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				   IShaderShadow* pShaderShadow,
				   NeoMotionModel_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
				   CBasePerMaterialContextData **pContextDataPtr );

#endif // NEO_THERMAL_MODEL_DX9_HELPER_H
