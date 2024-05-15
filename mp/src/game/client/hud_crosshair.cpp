//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_crosshair.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"
#include "ivrenderview.h"
#include "materialsystem/imaterialsystem.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "client_virtualreality.h"
#include "sourcevr/isourcevirtualreality.h"

#ifdef SIXENSE
#include "sixense/in_sixense.h"
#endif

#ifdef PORTAL
#include "c_portal_player.h"
#endif // PORTAL

#ifdef NEO
#include "weapon_neobasecombatweapon.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar crosshair( "crosshair", "1", FCVAR_ARCHIVE );
ConVar cl_observercrosshair( "cl_observercrosshair", "1", FCVAR_ARCHIVE );

using namespace vgui;

int ScreenTransform( const Vector& point, Vector& screen );

#ifdef TF_CLIENT_DLL
// If running TF, we use CHudTFCrosshair instead (which is derived from CHudCrosshair)
#else
DECLARE_HUDELEMENT_DEPTH(CHudCrosshair, 9999);
#endif

CHudCrosshair::CHudCrosshair( const char *pElementName ) :
		CHudElement( pElementName ), BaseClass( NULL, "HudCrosshair" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCrosshair = 0;

	m_clrCrosshair = Color( 0, 0, 0, 0 );

	m_vecCrossHairOffsetAngle.Init();

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR );
}

CHudCrosshair::~CHudCrosshair()
{
}

void CHudCrosshair::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_pDefaultCrosshair = gHUD.GetIcon("crosshair_default");
	SetPaintBackgroundEnabled( false );

    SetSize( ScreenWidth(), ScreenHeight() );

	SetForceStereoRenderToFrameBuffer( true );
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudCrosshair::ShouldDraw( void )
{
	bool bNeedsDraw;

	if ( m_bHideCrosshair )
		return false;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon && !pWeapon->ShouldDrawCrosshair() )
		return false;

#ifdef PORTAL
	C_Portal_Player *portalPlayer = ToPortalPlayer(pPlayer);
	if ( portalPlayer && portalPlayer->IsSuppressingCrosshair() )
		return false;
#endif // PORTAL

	/* disabled to avoid assuming it's an HL2 player.
	// suppress crosshair in zoom.
	if ( pPlayer->m_HL2Local.m_bZooming )
		return false;
	*/

	// draw a crosshair only if alive or spectating in eye
	if ( IsX360() )
	{
		bNeedsDraw = m_pCrosshair && 
			!engine->IsDrawingLoadingImage() &&
			!engine->IsPaused() && 
			( !pPlayer->IsSuitEquipped() || g_pGameRules->IsMultiplayer() ) &&
			g_pClientMode->ShouldDrawCrosshair() &&
			!( pPlayer->GetFlags() & FL_FROZEN ) &&
			( pPlayer->entindex() == render->GetViewEntity() ) &&
			( pPlayer->IsAlive() ||	( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) || ( cl_observercrosshair.GetBool() && pPlayer->GetObserverMode() == OBS_MODE_ROAMING ) );
	}
	else
	{
		bNeedsDraw = m_pCrosshair && 
			crosshair.GetInt() &&
			!engine->IsDrawingLoadingImage() &&
			!engine->IsPaused() && 
			g_pClientMode->ShouldDrawCrosshair() &&
			!( pPlayer->GetFlags() & FL_FROZEN ) &&
			( pPlayer->entindex() == render->GetViewEntity() ) &&
			!pPlayer->IsInVGuiInputMode() &&
			( pPlayer->IsAlive() ||	( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) || ( cl_observercrosshair.GetBool() && pPlayer->GetObserverMode() == OBS_MODE_ROAMING ) );
	}

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

#ifdef TF_CLIENT_DLL
extern ConVar cl_crosshair_red;
extern ConVar cl_crosshair_green;
extern ConVar cl_crosshair_blue;
extern ConVar cl_crosshair_scale;
#endif


void CHudCrosshair::GetDrawPosition ( float *pX, float *pY, bool *pbBehindCamera, QAngle angleCrosshairOffset )
{
	QAngle curViewAngles = CurrentViewAngles();
	Vector curViewOrigin = CurrentViewOrigin();

	int vx, vy, vw, vh;
	vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

	float screenWidth = vw;
	float screenHeight = vh;

	float x = screenWidth / 2;
	float y = screenHeight / 2;

	bool bBehindCamera = false;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( ( pPlayer != NULL ) && ( pPlayer->GetObserverMode()==OBS_MODE_NONE ) )
	{
		bool bUseOffset = false;
		
		Vector vecStart;
		Vector vecEnd;

		if ( UseVR() )
		{
			// These are the correct values to use, but they lag the high-speed view data...
			vecStart = pPlayer->Weapon_ShootPosition();
			Vector vecAimDirection = pPlayer->GetAutoaimVector( 1.0f );
			// ...so in some aim modes, they get zapped by something completely up-to-date.
			g_ClientVirtualReality.OverrideWeaponHudAimVectors ( &vecStart, &vecAimDirection );
			vecEnd = vecStart + vecAimDirection * MAX_TRACE_LENGTH;

			bUseOffset = true;
		}

#ifdef SIXENSE
		// TODO: actually test this Sixsense code interaction with things like HMDs & stereo.
        if ( g_pSixenseInput->IsEnabled() && !UseVR() )
		{
			// Never autoaim a predicted weapon (for now)
			vecStart = pPlayer->Weapon_ShootPosition();
			Vector aimVector;
			AngleVectors( CurrentViewAngles() - g_pSixenseInput->GetViewAngleOffset(), &aimVector );
			// calculate where the bullet would go so we can draw the cross appropriately
			vecEnd = vecStart + aimVector * MAX_TRACE_LENGTH;
			bUseOffset = true;
		}
#endif

		if ( bUseOffset )
		{
			trace_t tr;
			UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

			Vector screen;
			screen.Init();
			bBehindCamera = ScreenTransform(tr.endpos, screen) != 0;

			x = 0.5f * ( 1.0f + screen[0] ) * screenWidth + 0.5f;
			y = 0.5f * ( 1.0f - screen[1] ) * screenHeight + 0.5f;
		}
	}

	// MattB - angleCrosshairOffset is the autoaim angle.
	// if we're not using autoaim, just draw in the middle of the 
	// screen
	if ( angleCrosshairOffset != vec3_angle )
	{
		QAngle angles;
		Vector forward;
		Vector point, screen;

		// this code is wrong
		angles = curViewAngles + angleCrosshairOffset;
		AngleVectors( angles, &forward );
		VectorAdd( curViewOrigin, forward, point );
		ScreenTransform( point, screen );

		x += 0.5f * screen[0] * screenWidth + 0.5f;
		y += 0.5f * screen[1] * screenHeight + 0.5f;
	}

	*pX = x;
	*pY = y;
	*pbBehindCamera = bBehindCamera;
}

// NEO TODO (Rain): move to server game rules for server op toggling
ConVar cl_neo_scope_restrict_to_rectangle("cl_neo_scope_restrict_to_rectangle", "1", FCVAR_CHEAT,
	"Whether to enforce rectangular sniper scope shape regardless of screen ratio.", true, 0.0, true, 1.0);

void CHudCrosshair::Paint( void )
{
	if ( !m_pCrosshair )
		return;

	if ( !IsCurrentViewAccessAllowed() )
		return;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	float x, y;
	bool bBehindCamera;
	GetDrawPosition ( &x, &y, &bBehindCamera, m_vecCrossHairOffsetAngle );

	if( bBehindCamera )
		return;

#ifdef TF_CLIENT_DLL
	float flWeaponScale = 1.f;
#else
	float flWeaponScale_X = 1.0f;
	float flWeaponScale_Y = 1.0f;
#endif
	int iTextureW = m_pCrosshair->Width();
	int iTextureH = m_pCrosshair->Height();

	//DevMsg("TexFileW: %s (%s) :: W %d H %d\n", m_pCrosshair->szTextureFile, m_pCrosshair->szShortName, iTextureW, iTextureH);

	if (iTextureH == 0 || iTextureW == 0)
	{
		Assert(false);
		return;
	}

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	CNEOBaseCombatWeapon* pNeoWep = NULL;
	if ( pWeapon )
	{
		// NEO HACK (Rain): this should get implemented in virtual pNeoWep->GetWeaponCrosshairScale
		pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWeapon);
		if (pNeoWep && pNeoWep->GetNeoWepBits() & NEO_WEP_SCOPEDWEAPON)
		{
			int screenWidth, screenHeight;
			GetHudSize(screenWidth, screenHeight);
			Assert(screenWidth > 0 && screenHeight > 0);

			// Scale scope size based on resolution.
			flWeaponScale_X = static_cast<float>(screenWidth) / iTextureW;
			flWeaponScale_Y = static_cast<float>(screenHeight) / iTextureH;

			if (cl_neo_scope_restrict_to_rectangle.GetBool())
			{
// This is how many see-through "lens" pixels there are in X vs Y axes of the default scope03 texture.
#define RECT_SCOPE03_VISIBLE_AREA_SCALING (720.0f / 960.0f)
				// Get the smaller dimension as base for the square scope.
				flWeaponScale_X = Min(flWeaponScale_X, flWeaponScale_Y);
				// And further downscale with this ratio to make the actual scope lens part of the texture square shaped.
				flWeaponScale_Y = flWeaponScale_X * RECT_SCOPE03_VISIBLE_AREA_SCALING;
				// We can increase the final rectangle scale size by about this much without overflowing the screen area.
				flWeaponScale_X *= (1.0f + (1.0f - RECT_SCOPE03_VISIBLE_AREA_SCALING));
				flWeaponScale_Y *= (1.0f + (1.0f - RECT_SCOPE03_VISIBLE_AREA_SCALING));
			}
		}
		else
		{
			pWeapon->GetWeaponCrosshairScale(flWeaponScale_X);
			flWeaponScale_Y = flWeaponScale_X;
		}
	}

#ifdef TF_CLIENT_DLL
	float flPlayerScale = 1.0f;
	Color clr( cl_crosshair_red.GetInt(), cl_crosshair_green.GetInt(), cl_crosshair_blue.GetInt(), 255 );
	flPlayerScale = cl_crosshair_scale.GetFloat() / 32.0f;  // the player can change the scale in the options/multiplayer tab
	float flWidth = flWeaponScale * flPlayerScale * (float)iTextureW;
	float flHeight = flWeaponScale * flPlayerScale * (float)iTextureH;
	int iWidth = (int)(flWidth + 0.5f);
	int iHeight = (int)(flHeight + 0.5f);
	int iX = (int)(x + 0.5f);
	int iY = (int)(y + 0.5f);
	Color clr = m_clrCrosshair;
#endif

	float flWidth = flWeaponScale_X * (float)iTextureW;
	float flHeight = flWeaponScale_Y * (float)iTextureH;

	int iWidth = (int)( flWidth + 0.5f );
	int iHeight = (int)( flHeight + 0.5f );
	int iX = (int)( x + 0.5f );
	int iY = (int)( y + 0.5f );

	m_pCrosshair->DrawSelfCropped (
		iX-(iWidth/2), iY-(iHeight/2),
		0, 0,
		iTextureW, iTextureH,
		iWidth, iHeight,
#ifdef TF_CLIENT_DLL
		clr
#else
		m_clrCrosshair
#endif
	);

	if (pNeoWep && pNeoWep->GetNeoWepBits() & NEO_WEP_SCOPEDWEAPON)
	{
		int screenWidth, screenHeight;
		GetHudSize(screenWidth, screenHeight);

		const unsigned int prevCornerFlags = GetRoundedCorners();
		SetRoundedCorners(0);

		// NEO TODO (Rain): the NT scope does a color slide around RGB 0-20,
		// instead of being completely black. For now, just picking a close-ish color
		// instead of accurately representing. Should probably pick samples of the
		// scope texture around the edges, and use those for fill color, instead.
		// Stretching the scope wouldn't be a great workaround, because it ties the scope
		// FOV to screen resolution, which can give an unfair advantage on widescreen
		// or multi-monitor setups.
		const Color blockColor = Color(10, 10, 10, 255);

		// Draw black box on left side of the scope.
		DrawBox(0, 0, iX - (iWidth / 2), screenHeight, blockColor, 1.0f);
		// Draw black box on right side of the scope.
		DrawBox((iX - (iWidth / 2) + iWidth), 0, screenWidth, screenHeight, blockColor, 1.0f);
		// Draw black box on top of the scope.
		DrawBox(iX - (iWidth / 2), 0, iWidth, iY - (iHeight / 2), blockColor, 1.0f);
		// Draw black box under the scope.
		DrawBox(iX - (iWidth / 2), screenHeight - (iY - (iHeight / 2)), iWidth, screenHeight, blockColor, 1.0f);

		// Reset corner rounding.
		SetRoundedCorners(prevCornerFlags);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshairAngle( const QAngle& angle )
{
	VectorCopy( angle, m_vecCrossHairOffsetAngle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshair( CHudTexture *texture, const Color& clr )
{
	m_pCrosshair = texture;
	m_clrCrosshair = clr;
}

//-----------------------------------------------------------------------------
// Purpose: Resets the crosshair back to the default
//-----------------------------------------------------------------------------
void CHudCrosshair::ResetCrosshair()
{
	SetCrosshair( m_pDefaultCrosshair, Color(255, 255, 255, 255) );
}
