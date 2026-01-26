//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render a glowing outline around client renderable objects.
//
//===============================================================================

#include "cbase.h"
#include "glow_outline_effect.h"
#include "model_types.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "viewpostprocess.h"
#ifdef NEO
#include "neo_gamerules.h"
#include "neo_player_shared.h"
#endif // NEO

#define FULL_FRAME_TEXTURE "_rt_FullFrameFB"

#ifdef GLOWS_ENABLE

#ifdef NEO
ConVar glow_outline_effect_enable("glow_outline_effect_enable", "1", FCVAR_ARCHIVE, "Enable entity outline glow effects.", 
	[](IConVar* var [[maybe_unused]], const char* pOldValue [[maybe_unused]], float flOldValue [[maybe_unused]])
	{
		C_NEO_Player* pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
		if (pLocalPlayer) {
			pLocalPlayer->UpdateGlowEffects(pLocalPlayer->GetTeamNumber());
		}
	}
);
ConVar glow_outline_effect_width( "glow_outline_effect_width", "1.f", FCVAR_ARCHIVE, "Width of glow outline effect.", true, 0.f, false, 0.f);
ConVar glow_outline_effect_alpha( "glow_outline_effect_alpha", "1.f", FCVAR_ARCHIVE, "Alpha of glow outline effect.", true, 0.f, true, 1.f);
ConVar glow_outline_effect_center_alpha("glow_outline_effect_center_alpha", "0.1f", FCVAR_ARCHIVE, "Opacity of the part of the glow effect drawn on top of the player model when obstructed", true, 0.f, true, 1.f);
ConVar glow_outline_effect_textured_center_alpha("glow_outline_effect_textured_center_alpha", "0.2f", FCVAR_ARCHIVE, "Opacity of the part of the glow effect drawn on top of the player model when cloaked", true, 0.f, true, 1.f);
#else
ConVar glow_outline_effect_enable( "glow_outline_effect_enable", "0", 0, "Enable entity outline glow effects.");
ConVar glow_outline_effect_width( "glow_outline_width", "10.0f", FCVAR_CHEAT, "Width of glow outline effect in screen space." );
#endif // NEO

extern bool g_bDumpRenderTargets; // in viewpostprocess.cpp

CGlowObjectManager g_GlowObjectManager;

struct ShaderStencilState_t
{
	bool m_bEnable;
	StencilOperation_t m_FailOp;
	StencilOperation_t m_ZFailOp;
	StencilOperation_t m_PassOp;
	StencilComparisonFunction_t m_CompareFunc;
	int m_nReferenceValue;
	uint32 m_nTestMask;
	uint32 m_nWriteMask;

	ShaderStencilState_t()
	{
		m_bEnable = false;
		m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
		m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		m_nReferenceValue = 0;
		m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
	}

	void SetStencilState( CMatRenderContextPtr &pRenderContext  )
	{
		pRenderContext->SetStencilEnable( m_bEnable );
		pRenderContext->SetStencilFailOperation( m_FailOp );
		pRenderContext->SetStencilZFailOperation( m_ZFailOp );
		pRenderContext->SetStencilPassOperation( m_PassOp );
		pRenderContext->SetStencilCompareFunction( m_CompareFunc );
		pRenderContext->SetStencilReferenceValue( m_nReferenceValue );
		pRenderContext->SetStencilTestMask( m_nTestMask );
		pRenderContext->SetStencilWriteMask( m_nWriteMask );
	}
};

void CGlowObjectManager::RenderGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot )
{
	if ( g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		if ( glow_outline_effect_enable.GetBool() )
		{
			CMatRenderContextPtr pRenderContext( materials );

			int nX, nY, nWidth, nHeight;
			pRenderContext->GetViewport( nX, nY, nWidth, nHeight );

			PIXEvent _pixEvent( pRenderContext, "EntityGlowEffects" );
#ifdef NEO
			ApplyEntityGlowEffects(pSetup, nSplitScreenSlot, pRenderContext, 0.f, nX, nY, nWidth, nHeight);
#else
			ApplyEntityGlowEffects( pSetup, nSplitScreenSlot, pRenderContext, glow_outline_effect_width.GetFloat(), nX, nY, nWidth, nHeight );
#endif
		}
	}
}

static void SetRenderTargetAndViewPort( ITexture *rt, int w, int h )
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->SetRenderTarget(rt);
	pRenderContext->Viewport(0,0,w,h);
}

void CGlowObjectManager::RenderGlowModels( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext )
{
	//==========================================================================================//
	// This renders solid pixels with the correct coloring for each object that needs the glow.	//
	// After this function returns, this image will then be blurred and added into the frame	//
	// buffer with the objects stenciled out.													//
	//==========================================================================================//
	pRenderContext->PushRenderTargetAndViewport();

	// Save modulation color and blend
	Vector vOrigColor;
	render->GetColorModulation( vOrigColor.Base() );
	float flOrigBlend = render->GetBlend();

	// Get pointer to FullFrameFB
	ITexture *pRtFullFrame = NULL;
	pRtFullFrame = materials->FindTexture( FULL_FRAME_TEXTURE, TEXTURE_GROUP_RENDER_TARGET );

	SetRenderTargetAndViewPort( pRtFullFrame, pSetup->width, pSetup->height );

	pRenderContext->ClearColor3ub( 0, 0, 0 );
	pRenderContext->ClearBuffers( true, false, false );

	// Set override material for glow color
	IMaterial *pMatGlowColor = NULL;

	pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
#ifndef NEO
	g_pStudioRender->ForcedMaterialOverride( pMatGlowColor );
#endif

	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = false;
	stencilState.m_nReferenceValue = 0;
	stencilState.m_nTestMask = 0xFF;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_KEEP;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_KEEP;

	stencilState.SetStencilState( pRenderContext );

	//==================//
	// Draw the objects //
	//==================//
	for ( int i = 0; i < m_GlowObjectDefinitions.Count(); ++ i )
	{
		if ( m_GlowObjectDefinitions[i].IsUnused() || !m_GlowObjectDefinitions[i].ShouldDraw( nSplitScreenSlot ) )
			continue;

#ifdef NEO
		// DrawModel can call ForcedMaterialOverride also
		g_pStudioRender->ForcedMaterialOverride(pMatGlowColor);
#endif
		render->SetBlend( m_GlowObjectDefinitions[i].m_flGlowAlpha );
		Vector vGlowColor = m_GlowObjectDefinitions[i].m_vGlowColor * m_GlowObjectDefinitions[i].m_flGlowAlpha;
		render->SetColorModulation( &vGlowColor[0] ); // This only sets rgb, not alpha

		m_GlowObjectDefinitions[i].DrawModel();
	}

	if ( g_bDumpRenderTargets )
	{
		DumpTGAofRenderTarget( pSetup->width, pSetup->height, "GlowModels" );
	}

	g_pStudioRender->ForcedMaterialOverride( NULL );
	render->SetColorModulation( vOrigColor.Base() );
	render->SetBlend( flOrigBlend );
	
	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	stencilStateDisable.SetStencilState( pRenderContext );

	pRenderContext->PopRenderTargetAndViewport();
}

void CGlowObjectManager::ApplyEntityGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext, float flBloomScale, int x, int y, int w, int h )
{
	//=======================================================//
	// Render objects into stencil buffer					 //
	//=======================================================//
	// Set override shader to the same simple shader we use to render the glow models
	IMaterial *pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
	g_pStudioRender->ForcedMaterialOverride( pMatGlowColor );

	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	float flSavedBlend = render->GetBlend();

	// Set alpha to 0 so we don't touch any color pixels
	render->SetBlend( 0.0f );
	pRenderContext->OverrideDepthEnable( true, false );

	int iNumGlowObjects = 0;

	for ( int i = 0; i < m_GlowObjectDefinitions.Count(); ++ i )
	{
		if ( m_GlowObjectDefinitions[i].IsUnused() || !m_GlowObjectDefinitions[i].ShouldDraw( nSplitScreenSlot ) )
			continue;

		if ( m_GlowObjectDefinitions[i].m_bRenderWhenOccluded || m_GlowObjectDefinitions[i].m_bRenderWhenUnoccluded )
		{
			if ( m_GlowObjectDefinitions[i].m_bRenderWhenOccluded && m_GlowObjectDefinitions[i].m_bRenderWhenUnoccluded )
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 1;
#ifdef NEO
				if (m_GlowObjectDefinitions[i].m_bUseTexturedHighlight)
				{
					stencilState.m_nReferenceValue += NEO_GLOW_CLOAKED;
				}
				stencilState.m_nWriteMask = 1 + NEO_GLOW_CLOAKED;
#endif // NEO
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
				stencilState.m_PassOp = STENCILOPERATION_REPLACE;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState( pRenderContext );

				m_GlowObjectDefinitions[i].DrawModel();
			}
			else if ( m_GlowObjectDefinitions[i].m_bRenderWhenOccluded )
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
#ifdef NEO
				stencilState.m_nReferenceValue = NEO_GLOW_OBSTRUCTED;
#else
				stencilState.m_nReferenceValue = 1;
#endif // NEO
#ifdef NEO
				if (m_GlowObjectDefinitions[i].m_bUseTexturedHighlight)
				{
					stencilState.m_nReferenceValue += NEO_GLOW_CLOAKED;
				}
				stencilState.m_nWriteMask = NEO_GLOW_OBSTRUCTED + NEO_GLOW_CLOAKED;
#endif // NEO
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
				stencilState.m_PassOp = STENCILOPERATION_KEEP;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState( pRenderContext );
				
				m_GlowObjectDefinitions[i].DrawModel();
			}
			else if ( m_GlowObjectDefinitions[i].m_bRenderWhenUnoccluded )
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 2;
				stencilState.m_nTestMask = 0x1;
				stencilState.m_nWriteMask = 0x3;
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
				stencilState.m_PassOp = STENCILOPERATION_INCRSAT;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState( pRenderContext );
				
				m_GlowObjectDefinitions[i].DrawModel();
			}
		}

		iNumGlowObjects++;
	}

	// Need to do a 2nd pass to warm stencil for objects which are rendered only when occluded
	for ( int i = 0; i < m_GlowObjectDefinitions.Count(); ++ i )
	{
		if ( m_GlowObjectDefinitions[i].IsUnused() || !m_GlowObjectDefinitions[i].ShouldDraw( nSplitScreenSlot ) )
			continue;

		if ( m_GlowObjectDefinitions[i].m_bRenderWhenOccluded && !m_GlowObjectDefinitions[i].m_bRenderWhenUnoccluded )
		{
			ShaderStencilState_t stencilState;
			stencilState.m_bEnable = true;
#ifdef NEO
			stencilState.m_nReferenceValue = NEO_GLOW_NOTOBSTRUCTED;
#else
			stencilState.m_nReferenceValue = 2;
#endif // NEO
#ifdef NEO
			if (m_GlowObjectDefinitions[i].m_bUseTexturedHighlight)
			{
				stencilState.m_nReferenceValue += NEO_GLOW_CLOAKED;
			}
			stencilState.m_nWriteMask = NEO_GLOW_OBSTRUCTED + NEO_GLOW_NOTOBSTRUCTED + NEO_GLOW_CLOAKED;
#endif // NEO
			stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
			stencilState.m_PassOp = STENCILOPERATION_REPLACE;
			stencilState.m_FailOp = STENCILOPERATION_KEEP;
			stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
			stencilState.SetStencilState( pRenderContext );
			
			m_GlowObjectDefinitions[i].DrawModel();
		}
	}

	pRenderContext->OverrideDepthEnable( false, false );
	render->SetBlend( flSavedBlend );
	stencilStateDisable.SetStencilState( pRenderContext );
	g_pStudioRender->ForcedMaterialOverride( NULL );

	// If there aren't any objects to glow, don't do all this other stuff
	// this fixes a bug where if there are glow objects in the list, but none of them are glowing,
	// the whole screen blooms.
	if ( iNumGlowObjects <= 0 )
		return;

	//=============================================
	// Render the glow colors to _rt_FullFrameFB 
	//=============================================
	{
		PIXEvent pixEvent( pRenderContext, "RenderGlowModels" );
		RenderGlowModels( pSetup, nSplitScreenSlot, pRenderContext );
	}
	
	// Get viewport
#ifndef NEO
	int nSrcWidth = pSetup->width;
	int nSrcHeight = pSetup->height;
#endif // NEO
	int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
	pRenderContext->GetViewport( nViewportX, nViewportY, nViewportWidth, nViewportHeight );

	// Get material and texture pointers
#ifdef NEO
	ITexture *pRtQuarterSize1 = materials->FindTexture( FULL_FRAME_TEXTURE, TEXTURE_GROUP_RENDER_TARGET );
#else
	ITexture *pRtQuarterSize1 = materials->FindTexture( "_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET );
#endif //NEO

	{
		//=======================================================================================================//
		// At this point, pRtQuarterSize0 is filled with the fully colored glow around everything as solid glowy //
		// blobs. Now we need to stencil out the original objects by only writing pixels that have no            //
		// stencil bits set in the range we care about.                                                          //
		//=======================================================================================================//
		IMaterial *pMatHaloAddToScreen = materials->FindMaterial( "dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, true );
#ifdef NEO
		IMaterial *pMatHaloTexturedAddToScreen = materials->FindMaterial( "dev/halo_textured_add_to_screen", TEXTURE_GROUP_OTHER, true );
		IMaterial *pMatHaloAddToScreenOutline = materials->FindMaterial( "dev/halo_add_to_screen_outline", TEXTURE_GROUP_OTHER, true );
#endif // NEO

		// Do not fade the glows out at all (weight = 1.0)
		IMaterialVar *pDimVar = pMatHaloAddToScreen->FindVar( "$C0_X", NULL );
		pDimVar->SetFloatValue( 1.0f );

		// Set stencil state
		ShaderStencilState_t stencilState;
		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0x0; // We're not changing stencil
		stencilState.m_nTestMask = 0xFF; 
		stencilState.m_nReferenceValue = 0x0;
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
		stencilState.m_PassOp = STENCILOPERATION_KEEP;
		stencilState.m_FailOp = STENCILOPERATION_KEEP;
		stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
		stencilState.SetStencilState( pRenderContext );

		// Draw quad
#ifdef NEO
		const float outlineWidth = glow_outline_effect_width.GetFloat();
		const float outlineAlpha = glow_outline_effect_alpha.GetFloat();
		if (outlineWidth && outlineAlpha)
		{
			IMaterialVar* pOutlineVar = pMatHaloAddToScreenOutline->FindVar("$C0_X", NULL);
			pOutlineVar->SetFloatValue(outlineWidth);
			IMaterialVar* pAlphaVar = pMatHaloAddToScreenOutline->FindVar("$C0_Y", NULL);
			pAlphaVar->SetFloatValue(outlineAlpha);
			pRenderContext->DrawScreenSpaceRectangle(pMatHaloAddToScreenOutline, 0, 0, nViewportWidth+1, nViewportHeight+1,
				0, 0, pRtQuarterSize1->GetActualWidth(), pRtQuarterSize1->GetActualHeight(),
				pRtQuarterSize1->GetActualWidth(),
				pRtQuarterSize1->GetActualHeight());
		}

		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0;
		stencilState.m_nTestMask = NEO_GLOW_OBSTRUCTED + NEO_GLOW_VIEWMODEL;
		stencilState.m_nReferenceValue = NEO_GLOW_OBSTRUCTED;
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
		stencilState.SetStencilState( pRenderContext );

		const float alpha = glow_outline_effect_center_alpha.GetFloat();
		if (alpha)
		{
			stencilState.SetStencilState( pRenderContext );
			pDimVar->SetFloatValue(alpha);
			pRenderContext->DrawScreenSpaceRectangle(pMatHaloAddToScreen, 0, 0, nViewportWidth+1, nViewportHeight+1,
				0, 0, pRtQuarterSize1->GetActualWidth(), pRtQuarterSize1->GetActualHeight(),
				pRtQuarterSize1->GetActualWidth(),
				pRtQuarterSize1->GetActualHeight());
		}
		
		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0;
		stencilState.m_nTestMask = NEO_GLOW_CLOAKED + NEO_GLOW_VIEWMODEL;
		stencilState.m_nReferenceValue = NEO_GLOW_CLOAKED;
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
		stencilState.SetStencilState( pRenderContext );

		const float alpha2 = glow_outline_effect_textured_center_alpha.GetFloat();
		if (alpha2)
		{
			stencilState.SetStencilState( pRenderContext );
			IMaterialVar *pDimVar = pMatHaloTexturedAddToScreen->FindVar( "$C0_X", NULL );
			pDimVar->SetFloatValue(alpha2);

			int width, height;
			pRenderContext->GetWindowSize(width, height);
			IMaterialVar *pResX = pMatHaloTexturedAddToScreen->FindVar( "$C1_X", NULL );
			pResX->SetIntValue(width);
			IMaterialVar *pResY = pMatHaloTexturedAddToScreen->FindVar( "$C1_Y", NULL );
			pResY->SetIntValue(height);

			pRenderContext->DrawScreenSpaceRectangle(pMatHaloTexturedAddToScreen, 0, 0, nViewportWidth+1, nViewportHeight+1,
				0, 0, pRtQuarterSize1->GetActualWidth(), pRtQuarterSize1->GetActualHeight(),
				pRtQuarterSize1->GetActualWidth(),
				pRtQuarterSize1->GetActualHeight());
		}
#else
		pRenderContext->DrawScreenSpaceRectangle(pMatHaloAddToScreen, 0, 0, nViewportWidth, nViewportHeight,
			0.0f, -0.5f, nSrcWidth / 4 - 1, nSrcHeight / 4 - 1,
			pRtQuarterSize1->GetActualWidth(),
			pRtQuarterSize1->GetActualHeight() );

#endif // NEO
		stencilStateDisable.SetStencilState( pRenderContext );
	}
}

void CGlowObjectManager::GlowObjectDefinition_t::DrawModel()
{
	if ( m_hEntity.Get() )
	{
#ifdef NEO
		m_hEntity->DrawModel( STUDIO_RENDER | STUDIO_IGNORE_NEO_EFFECTS );
#else
		m_hEntity->DrawModel( STUDIO_RENDER );
#endif // NEO
		C_BaseEntity *pAttachment = m_hEntity->FirstMoveChild();

		while ( pAttachment != NULL )
		{
			if ( !g_GlowObjectManager.HasGlowEffect( pAttachment ) && pAttachment->ShouldDraw() )
			{
#ifdef NEO
				pAttachment->DrawModel( STUDIO_RENDER | STUDIO_IGNORE_NEO_EFFECTS );
#else
				pAttachment->DrawModel( STUDIO_RENDER );
#endif // NEO
			}
			pAttachment = pAttachment->NextMovePeer();
		}
	}
}

#ifdef NEO
// NEO TODO (Adam) sort m_GlowObjectDefinitions so objects further away get drawn first. m_GlowObjectDefinitions maintains a linked list and a nextFreeIndex, and so does each element
// and glowing objects hold a reference to m_GlowObjectDefinitions via CGlowObject so sorting it would require updating all of these, or maybe a second list could be maintained instead?
// alternatively we could do away with the linked list and make C_BaseEntity hold the reference instead of C_BaseCombatCharacter and C_TeamTrainWatcher.
// This would also allow us to make any entity glow instead of just these two classes (or any attachment of these two classes)

//void CGlowObjectManager::SortGlowEffects()
//{
//	C_NEO_Player* pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
//	if (!pLocalPlayer)
//	{
//		return;
//	}
//	static Vector localPlayerEyePosition = pLocalPlayer->EyePosition();
//
//	m_GlowObjectDefinitions.InPlaceQuickSort([](const GlowObjectDefinition_t* first, const GlowObjectDefinition_t* second)->int{ 
//		C_BaseEntity* firstEntity = first->m_hEntity.Get();
//		C_BaseEntity* secondEntity = second->m_hEntity.Get();
//		if (!firstEntity || !secondEntity)
//		{
//			return 0;
//		}
//
//		return secondEntity->GetAbsOrigin().DistToSqr(localPlayerEyePosition) - firstEntity->GetAbsOrigin().DistToSqr(localPlayerEyePosition);
//	});
//}

#endif // NEO
#endif // GLOWS_ENABLE
