//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_hl2mpnormal.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"

#ifdef NEO
	#include "neo/ui/neo_scoreboard.h"
	#include "neo/ui/neo_hud_elements.h"
#else
	#include "hl2mpclientscoreboard.h"
#endif

#ifdef NEO
	#include "weapon_neobasecombatweapon.h"
	#include "weapon_hl2mpbase.h"
	#include "hl2mp_weapon_parse.h"

	#include <mathlib/mathlib.h>
#endif

#include "hl2mptextwindow.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeHL2MPNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}

ClientModeHL2MPNormal* GetClientModeHL2MPNormal()
{
	Assert( dynamic_cast< ClientModeHL2MPNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeHL2MPNormal* >( GetClientModeNormal() );
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual IViewPortPanel *CreatePanelByName( const char *szPanelName );
};

int ClientModeHL2MPNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
	{
#ifdef NEO
		newpanel = new CNEOScoreBoard(this);
#else
		newpanel = new CHL2MPClientScoreBoardDialog( this );
#endif
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CHL2MPTextWindow( this );
		return newpanel;
	}
	else if ( Q_strcmp(PANEL_SPECGUI, szPanelName) == 0 )
	{
		newpanel = new CHL2MPSpectatorGUI( this );	
		return newpanel;
	}
#ifdef NEO
	else if (Q_strcmp(PANEL_NEO_HUD, szPanelName) == 0)
	{
		newpanel = new CNeoHudElements(this);
		return newpanel;
	}
#endif

	
	return BaseClass::CreatePanelByName( szPanelName ); 
}

//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeHL2MPNormal::ClientModeHL2MPNormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHL2MPNormal::~ClientModeHL2MPNormal()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeHL2MPNormal::Init()
{
	BaseClass::Init();

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/CombinePanelScheme.res", "CombineScheme" );
	if (!g_hVGuiCombineScheme)
	{
		Warning( "Couldn't load combine panel scheme!\n" );
	}
}

#ifdef NEO


ConVar cl_neo_decouple_vm_fov("cl_neo_decouple_vm_fov", "1", FCVAR_CHEAT, "Whether to decouple aim FOV from viewmodel FOV.", true, 0.0f, true, 1.0f);
ConVar cl_neo_decoupled_vm_fov_lerp_scale("cl_neo_decoupled_vm_fov_lerp_scale", "10", FCVAR_CHEAT, "Multiplier for decoupled FOV lerp speed.", true, 0.01, false, 0);

//-----------------------------------------------------------------------------
// Purpose: Use correct view model FOV
//-----------------------------------------------------------------------------
float ClientModeHL2MPNormal::GetViewModelFOV()
{
	if (cl_neo_decouple_vm_fov.GetBool())
	{
		// NEO TODO (Rain): switch to static cast when all weapons
		// inherit in such a way that downcast is guaranteed to be valid.
		auto pWeapon = dynamic_cast<C_NEOBaseCombatWeapon*>(GetActiveWeapon());
		if (pWeapon)
		{
			auto pOwner = static_cast<C_NEO_Player*>(pWeapon->GetOwner());
			// Should always have an owner if we've reached this far.
			Assert(pOwner);

			// While the adjust factor appears to be originally intended for LOD calculations,
			// it gives us a way to tie the vm fov lerp to player's actual zoom rate without having to
			// roll our own implementation.
			const float flScale = cl_neo_decoupled_vm_fov_lerp_scale.GetFloat() / MAX(0.1f, pOwner->GetFOVDistanceAdjustFactor());
			//DevMsg("GetViewModelFOV flScale: %.2f\n", flScale);

			const CHL2MPSWeaponInfo *pWepInfo = &pWeapon->GetHL2MPWpnData();
			Assert(pWepInfo);

			static float flCurrentFov = pWepInfo->m_flVMFov;
			const float flTargetFov = pOwner->IsInAim() ? (pWepInfo->m_flVMAimFov) : (pWepInfo->m_flVMFov);

			// Get delta time
			const float flThisTime = gpGlobals->curtime;
			static float flLastTime = flThisTime;
			const float flDeltaTime = flThisTime - flLastTime;
			flLastTime = flThisTime;

			// Lerp towards the desired fov
			flCurrentFov = Lerp(flDeltaTime * flScale, flCurrentFov, flTargetFov);

			// Snap to target when approximately equal
			const float flThreshold = 0.001f;
			if (fabs(flTargetFov - flCurrentFov) < flThreshold)
			{
				flCurrentFov = flTargetFov;
			}

			return flCurrentFov;
		}
	}

	return BaseClass::GetViewModelFOV();
}
#endif

