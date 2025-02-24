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
	#include "prediction.h"
	#include "neo/ui/neo_scoreboard.h"
	extern ConVar v_viewmodel_fov;
#else
	#include "hl2mpclientscoreboard.h"
#endif

#ifdef NEO
	#include "weapon_neobasecombatweapon.h"
	#include "weapon_hl2mpbase.h"
	#include "hl2mp_weapon_parse.h"

	#include "weapon_srs.h"
	#include "weapon_zr68l.h"
	//#include "weapon_m41l.h"

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

IViewPortPanel* CHudViewport::CreatePanelByName(const char *szPanelName)
{
	IViewPortPanel* newpanel = NULL;

	if (Q_strcmp(PANEL_SCOREBOARD, szPanelName) == 0)
	{
#ifdef NEO
		newpanel = new CNEOScoreBoard(this);
#else
		newpanel = new CHL2MPClientScoreBoardDialog( this );
#endif
		return newpanel;
	}
	else if (Q_strcmp(PANEL_INFO, szPanelName) == 0)
	{
		newpanel = new CHL2MPTextWindow(this);
		return newpanel;
	}
	else if (Q_strcmp(PANEL_SPECGUI, szPanelName) == 0)
	{
		newpanel = new CHL2MPSpectatorGUI(this);
		return newpanel;
	}

	return BaseClass::CreatePanelByName(szPanelName);
}

//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeHL2MPNormal::ClientModeHL2MPNormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start(gameuifuncs, gameeventmanager);
	m_flStartAimingChange = 0.0f;
	m_bViewAim = false;
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
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx(enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/CombinePanelScheme.res", "CombineScheme");
	if (!g_hVGuiCombineScheme)
	{
		Warning("Couldn't load combine panel scheme!\n");
	}

#ifdef NEO
	const char* neoKillfeedIcons[][2] = {
		{ "neo_bus",			"vgui/hud/kill_shortbus.vmt" },
		{ "neo_gun",			"vgui/hud/kill_gun.vmt" },
		{ "neo_hs",				"vgui/hud/kill_headshot.vmt" },
		{ "neo_boom",			"vgui/hud/kill_explode.vmt" },
		{ "neo_aa13",			"vgui/hud/weapons/aa13.vmt" },
		{ "neo_ghost",			"vgui/hud/weapons/ghost.vmt" },
		{ "neo_grenade",		"vgui/hud/weapons/grenade.vmt" },
		{ "neo_jitte",			"vgui/hud/weapons/jitte.vmt" },
		{ "neo_jittescoped",	"vgui/hud/weapons/jittescoped.vmt" },
		{ "neo_knife",			"vgui/hud/weapons/knife.vmt" },
		{ "neo_kyla",			"vgui/hud/weapons/kyla.vmt" },
		{ "neo_m41",			"vgui/hud/weapons/m41.vmt" },
		{ "neo_m41l",			"vgui/hud/weapons/m41l.vmt" },
		{ "neo_m41s",			"vgui/hud/weapons/m41s.vmt" },
		{ "neo_milso",			"vgui/hud/weapons/milso.vmt" },
		{ "neo_mpn",			"vgui/hud/weapons/mpn.vmt" },
		{ "neo_mpn_unsilenced",	"vgui/hud/weapons/mpn_unsilenced.vmt" },
		{ "neo_mx",				"vgui/hud/weapons/mx.vmt" },
		{ "neo_mx_silenced",	"vgui/hud/weapons/mx_silenced" },
		{ "neo_pz",				"vgui/hud/weapons/pz.vmt" },
		{ "neo_remotedet",		"vgui/hud/weapons/remotedet.vmt" },
		{ "neo_smac",			"vgui/hud/weapons/smac.vmt" },
		{ "neo_smokegrenade",	"vgui/hud/weapons/smokegrenade.vmt" },
		{ "neo_spidermine",		"vgui/hud/weapons/spidermine.vmt" },
		{ "neo_srm",			"vgui/hud/weapons/srm.vmt" },
		{ "neo_srm_s",			"vgui/hud/weapons/srm_s.vmt" },
		{ "neo_srs",			"vgui/hud/weapons/srs.vmt" },
		{ "neo_supa7",			"vgui/hud/weapons/supa7.vmt" },
		{ "neo_tachi",			"vgui/hud/weapons/tachi.vmt" },
		{ "neo_zr68c",			"vgui/hud/weapons/zr68c.vmt" },
		{ "neo_zr68l",			"vgui/hud/weapons/zr68l.vmt" },
		{ "neo_zr68s",			"vgui/hud/weapons/zr68s.vmt" },
#ifdef INCLUDE_WEP_PBK
		{ "neo_pbk56s",			"vgui/hud/weapons/pbk56s.vmt" },
#endif // INCLUDE_WEP_PBK
	};

	for (int i = 0; i < ARRAYSIZE(neoKillfeedIcons); ++i)
	{
		AssertValidStringPtr(neoKillfeedIcons[i][0]);
		AssertValidStringPtr(neoKillfeedIcons[i][1]);
		PrecacheMaterial(neoKillfeedIcons[i][1]);
		Assert(materials->FindMaterial(neoKillfeedIcons[i][1], TEXTURE_GROUP_PRECACHED));

		CHudTexture neoKillFeedTex;
		neoKillFeedTex.bRenderUsingFont = false;
		V_strcpy_safe(neoKillFeedTex.szShortName, neoKillfeedIcons[i][0]);
		V_strcpy_safe(neoKillFeedTex.szTextureFile, neoKillfeedIcons[i][1]);

		gHUD.AddSearchableHudIconToList(neoKillFeedTex);
	}
#endif
}

#ifdef NEO
ConVar cl_neo_decouple_vm_fov("cl_neo_decouple_vm_fov", "1", FCVAR_CHEAT, "Whether to decouple aim FOV from viewmodel FOV.", true, false, true, true);
ConVar neo_viewmodel_fov_offset("neo_viewmodel_fov_offset", "0", FCVAR_ARCHIVE, "Sets the field-of-view offset for the viewmodel.", true, -20.0f, true, 40.0f);

//-----------------------------------------------------------------------------
// Purpose: Use correct view model FOV
//-----------------------------------------------------------------------------
float ClientModeHL2MPNormal::GetViewModelFOV()
{
	if (!cl_neo_decouple_vm_fov.GetBool())
	{
		return BaseClass::GetViewModelFOV();
	}

	Assert(!GetActiveWeapon() || dynamic_cast<C_NEOBaseCombatWeapon*>(GetActiveWeapon()));
	auto pWeapon = static_cast<C_NEOBaseCombatWeapon*>(GetActiveWeapon());
	if (!pWeapon)
	{
		return BaseClass::GetViewModelFOV();
	}

	auto pOwner = static_cast<C_NEO_Player*>(pWeapon->GetOwner());
	if (!pOwner)
	{
		return BaseClass::GetViewModelFOV();
	}

	auto pVm = pOwner->GetViewModel();
	if (pVm)
	{
		// Toggle sniper viewmodel rendering according to scoped status.
		if (pWeapon->GetNeoWepBits() & NEO_WEP_SCOPEDWEAPON)
		{
			if (pOwner->IsInAim())
			{
				pVm->AddEffects(EF_NODRAW);
			}
			else
			{
				pVm->RemoveEffects(EF_NODRAW);
			}
		}
	}

	float flTargetFov = m_flVMFOV;
	if (!prediction->InPrediction())
	{
		const bool playerAiming = pOwner->IsInAim();
		const float currentTime = gpGlobals->curtime;
		if (m_bViewAim && !playerAiming)
		{
			// From aiming to not aiming
			m_flStartAimingChange = currentTime;
			m_bViewAim = false;
		}
		else if (!m_bViewAim && playerAiming)
		{
			// From not aiming to aiming
			m_flStartAimingChange = currentTime;
			m_bViewAim = true;
		}

		const CHL2MPSWeaponInfo* pWepInfo = &pWeapon->GetHL2MPWpnData();
		Assert(pWepInfo);

		const float endAimingChange = m_flStartAimingChange + NEO_ZOOM_SPEED;
		const bool inAimingChange = (m_flStartAimingChange <= currentTime && currentTime < endAimingChange);
		if (inAimingChange)
		{
			float percentage = clamp((currentTime - m_flStartAimingChange) / NEO_ZOOM_SPEED, 0.0f, 1.0f);
			if (playerAiming) percentage = 1.0f - percentage;
			flTargetFov = Lerp(percentage, pWepInfo->m_flVMAimFov, pWepInfo->m_flVMFov);
		}
		else
		{
			flTargetFov = playerAiming ? (pWepInfo->m_flVMAimFov) : (pWepInfo->m_flVMFov);
		}
		m_flVMFOV = flTargetFov;
	}
	return flTargetFov + neo_viewmodel_fov_offset.GetFloat();
}
#endif
