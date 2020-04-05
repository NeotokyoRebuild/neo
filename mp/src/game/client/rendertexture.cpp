//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// Implements local hooks into named renderable textures.
// See matrendertexture.cpp in material system for list of available RT's
//
//=============================================================================//

#include "materialsystem/imesh.h"
#include "materialsystem/itexture.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "tier1/strtools.h"
#include "rendertexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void ReleaseRenderTargets( void );

void AddReleaseFunc( void )
{
	static bool bAdded = false;
	if( !bAdded )
	{
		bAdded = true;
		materials->AddReleaseFunc( ReleaseRenderTargets );
	}
}

//=============================================================================
// Power of Two Frame Buffer Texture
//=============================================================================
static CTextureReference s_pPowerOfTwoFrameBufferTexture;
ITexture *GetPowerOfTwoFrameBufferTexture( void )
{
	if ( IsX360() )
	{
		return GetFullFrameFrameBufferTexture( 1 );
	}

	if ( !s_pPowerOfTwoFrameBufferTexture )
	{
		s_pPowerOfTwoFrameBufferTexture.Init( materials->FindTexture( "_rt_PowerOfTwoFB", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pPowerOfTwoFrameBufferTexture ) );
		AddReleaseFunc();
	}
	
	return s_pPowerOfTwoFrameBufferTexture;
}

//=============================================================================
// Fullscreen Texture
//=============================================================================
static CTextureReference s_pFullscreenTexture;
ITexture *GetFullscreenTexture( void )
{
	if ( !s_pFullscreenTexture )
	{
		s_pFullscreenTexture.Init( materials->FindTexture( "_rt_Fullscreen", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pFullscreenTexture ) );
		AddReleaseFunc();
	}

	return s_pFullscreenTexture;
}

//=============================================================================
// Camera Texture
//=============================================================================
static CTextureReference s_pCameraTexture;
ITexture *GetCameraTexture( void )
{
	if ( !s_pCameraTexture )
	{
		s_pCameraTexture.Init( materials->FindTexture( "_rt_Camera", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pCameraTexture ) );
		AddReleaseFunc();
	}
	
	return s_pCameraTexture;
}

//=============================================================================
// Full Frame Depth Texture
//=============================================================================
static CTextureReference s_pFullFrameDepthTexture;
ITexture *GetFullFrameDepthTexture( void )
{
	if ( !s_pFullFrameDepthTexture )
	{
		s_pFullFrameDepthTexture.Init( materials->FindTexture( "_rt_FullFrameDepth", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pFullFrameDepthTexture ) );
		AddReleaseFunc();
	}

	return s_pFullFrameDepthTexture;
}

//=============================================================================
// Full Frame Buffer Textures
//=============================================================================
static CTextureReference s_pFullFrameFrameBufferTexture[MAX_FB_TEXTURES];
ITexture *GetFullFrameFrameBufferTexture( int textureIndex )
{
	if ( !s_pFullFrameFrameBufferTexture[textureIndex] )
	{
		char name[256];
		if( textureIndex != 0 )
		{
			V_sprintf_safe( name, "_rt_FullFrameFB%d", textureIndex );
		}
		else
		{
			V_strcpy_safe( name, "_rt_FullFrameFB" );
		}
		s_pFullFrameFrameBufferTexture[textureIndex].Init( materials->FindTexture( name, TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pFullFrameFrameBufferTexture[textureIndex] ) );
		AddReleaseFunc();
	}
	
	return s_pFullFrameFrameBufferTexture[textureIndex];
}


//=============================================================================
// Water reflection
//=============================================================================
static CTextureReference s_pWaterReflectionTexture;
ITexture *GetWaterReflectionTexture( void )
{
	if ( !s_pWaterReflectionTexture )
	{
		s_pWaterReflectionTexture.Init( materials->FindTexture( "_rt_WaterReflection", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pWaterReflectionTexture ) );
		AddReleaseFunc();
	}
	
	return s_pWaterReflectionTexture;
}

//=============================================================================
// Water refraction
//=============================================================================
static CTextureReference s_pWaterRefractionTexture;
ITexture *GetWaterRefractionTexture( void )
{
	if ( !s_pWaterRefractionTexture )
	{
		s_pWaterRefractionTexture.Init( materials->FindTexture( "_rt_WaterRefraction", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pWaterRefractionTexture ) );
		AddReleaseFunc();
	}
	
	return s_pWaterRefractionTexture;
}

//=============================================================================
// Small Buffer HDR0
//=============================================================================
static CTextureReference s_pSmallBufferHDR0;
ITexture *GetSmallBufferHDR0( void )
{
	if ( !s_pSmallBufferHDR0 )
	{
		s_pSmallBufferHDR0.Init( materials->FindTexture( "_rt_SmallHDR0", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pSmallBufferHDR0 ) );
		AddReleaseFunc();
	}
	
	return s_pSmallBufferHDR0;
}

//=============================================================================
// Small Buffer HDR1
//=============================================================================
static CTextureReference s_pSmallBufferHDR1;
ITexture *GetSmallBufferHDR1( void )
{
	if ( !s_pSmallBufferHDR1 )
	{
		s_pSmallBufferHDR1.Init( materials->FindTexture( "_rt_SmallHDR1", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pSmallBufferHDR1 ) );
		AddReleaseFunc();
	}
	
	return s_pSmallBufferHDR1;
}

#ifdef NEO
static CTextureReference s_pSSAO;
ITexture *GetSSAO(void)
{
	if (!s_pSSAO)
	{
		s_pSSAO.Init(materials->FindTexture("_rt_SSAO", TEXTURE_GROUP_RENDER_TARGET));
		Assert(!IsErrorTexture(s_pSSAO));
		AddReleaseFunc();
	}
	else
	{
		Assert(!IsErrorTexture(s_pSSAO));
	}

	return s_pSSAO;
}

static CTextureReference s_pSSAO_IM;
ITexture *GetSSAOIntermediate(void)
{
	if (!s_pSSAO_IM)
	{
		s_pSSAO_IM.Init(materials->FindTexture("_rt_SSAO_Intermediate", TEXTURE_GROUP_RENDER_TARGET));
		Assert(s_pSSAO_IM && !s_pSSAO_IM->IsError());
		AddReleaseFunc();
	}
	else
	{
		Assert(!s_pSSAO_IM->IsError());
	}

	return s_pSSAO_IM;
}

static CTextureReference s_pTV;
ITexture* GetTV(void)
{
	if (!s_pTV)
	{
		s_pTV.Init(materials->FindTexture("_rt_ThermalVision", TEXTURE_GROUP_RENDER_TARGET));
		Assert(!IsErrorTexture(s_pTV));
		AddReleaseFunc();
	}
	else
	{
		Assert(!IsErrorTexture(s_pTV));
	}

	return s_pTV;
}

static CTextureReference s_pMV;
ITexture *GetMV(void)
{
	if (!s_pMV)
	{
		s_pMV.Init(materials->FindTexture("_rt_MotionVision", TEXTURE_GROUP_RENDER_TARGET));
		Assert(!IsErrorTexture(s_pMV));
		AddReleaseFunc();
	}
	else
	{
		Assert(!IsErrorTexture(s_pMV));
	}

	return s_pMV;
}

static CTextureReference s_pMV_Buffer1, s_pMV_Buffer2;
ITexture *GetMVBuffer(const int index)
{
	if (!s_pMV_Buffer1)
	{
		s_pMV_Buffer1.Init(materials->FindTexture("_rt_MotionVision_Buffer1", TEXTURE_GROUP_RENDER_TARGET));
		Assert(!IsErrorTexture(s_pMV_Buffer1));

		s_pMV_Buffer2.Init(materials->FindTexture("_rt_MotionVision_Buffer2", TEXTURE_GROUP_RENDER_TARGET));
		Assert(!IsErrorTexture(s_pMV_Buffer2));

		AddReleaseFunc();
	}
	else
	{
		Assert(s_pMV_Buffer1 && !IsErrorTexture(s_pMV_Buffer1));
		Assert(s_pMV_Buffer2 && !IsErrorTexture(s_pMV_Buffer2));
	}

	Assert(index == 0 || index == 1);

	return (index == 0) ? s_pMV_Buffer1 : s_pMV_Buffer2;
}

static CTextureReference s_pMV_IM;
ITexture *GetMVIntermediate(void)
{
	if (!s_pMV_IM)
	{
		s_pMV_IM.Init(materials->FindTexture("_rt_MotionVision_Intermediate", TEXTURE_GROUP_RENDER_TARGET));
		Assert(!IsErrorTexture(s_pMV_IM));
		AddReleaseFunc();
	}
	else
	{
		Assert(!IsErrorTexture(s_pMV_IM));
	}

	return s_pMV_IM;
}
#endif

//=============================================================================
// Quarter Sized FB0
//=============================================================================
static CTextureReference s_pQuarterSizedFB0;
ITexture *GetSmallBuffer0( void )
{
	if ( !s_pQuarterSizedFB0 )
	{
		s_pQuarterSizedFB0.Init( materials->FindTexture( "_rt_SmallFB0", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pQuarterSizedFB0 ) );
		AddReleaseFunc();
	}
	
	return s_pQuarterSizedFB0;
}

//=============================================================================
// Quarter Sized FB1
//=============================================================================
static CTextureReference s_pQuarterSizedFB1;
ITexture *GetSmallBuffer1( void )
{
	if ( !s_pQuarterSizedFB1 )
	{
		s_pQuarterSizedFB1.Init( materials->FindTexture( "_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pQuarterSizedFB1 ) );
		AddReleaseFunc();
	}
	
	return s_pQuarterSizedFB1;
}

//=============================================================================
// Teeny Textures
//=============================================================================
static CTextureReference s_TeenyTextures[MAX_TEENY_TEXTURES];
ITexture *GetTeenyTexture( int which )
{
	if ( IsX360() )
	{
		Assert( 0 );
		return NULL;
	}

	Assert( which < MAX_TEENY_TEXTURES );

	if ( !s_TeenyTextures[which] )
	{
		char nbuf[20];
		sprintf( nbuf, "_rt_TeenyFB%d", which );
		s_TeenyTextures[which].Init( materials->FindTexture( nbuf, TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_TeenyTextures[which] ) );
		AddReleaseFunc();
	}
	return s_TeenyTextures[which];
}

void ReleaseRenderTargets( void )
{
	s_pPowerOfTwoFrameBufferTexture.Shutdown();
	s_pCameraTexture.Shutdown();
	s_pWaterReflectionTexture.Shutdown();
	s_pWaterRefractionTexture.Shutdown();
	s_pQuarterSizedFB0.Shutdown();
	s_pQuarterSizedFB1.Shutdown();
	s_pFullFrameDepthTexture.Shutdown();

	for (int i=0; i<MAX_FB_TEXTURES; ++i)
		s_pFullFrameFrameBufferTexture[i].Shutdown();

#ifdef NEO
	s_pSSAO.Shutdown();
	s_pSSAO_IM.Shutdown();
	s_pMV.Shutdown();
	s_pMV_IM.Shutdown();
	s_pMV_Buffer1.Shutdown();
	s_pMV_Buffer2.Shutdown();
	s_pTV.Shutdown();
#endif
}
