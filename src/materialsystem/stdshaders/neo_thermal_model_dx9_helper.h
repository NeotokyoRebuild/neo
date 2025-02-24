//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
// Example shader modified for models in thermals
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
struct NeoThermalModel_DX9_Vars_t
{
	NeoThermalModel_DX9_Vars_t() { memset( this, 0xFF, sizeof(*this) ); }

	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nAlphaTestReference;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
};

void InitParamsNeoThermalModel_DX9( CBaseVSShader *pShader, IMaterialVar** params,
						 const char *pMaterialName, NeoThermalModel_DX9_Vars_t &info );

void InitNeoThermalModel_DX9( CBaseVSShader *pShader, IMaterialVar** params, 
				   NeoThermalModel_DX9_Vars_t &info );

void DrawNeoThermalModel_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				   IShaderShadow* pShaderShadow,
				   NeoThermalModel_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
				   CBasePerMaterialContextData **pContextDataPtr );

#endif // NEO_THERMAL_MODEL_DX9_HELPER_H
