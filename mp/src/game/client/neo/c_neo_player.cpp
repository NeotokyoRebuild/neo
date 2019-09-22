#include "cbase.h"
#include "vcollide_parse.h"
#include "c_neo_player.h"
#include "view.h"
#include "takedamageinfo.h"
#include "neo_gamerules.h"
#include "in_buttons.h"
#include "iviewrender_beams.h"			// flashlight beam
#include "r_efx.h"
#include "dlight.h"

#include "clientmode_hl2mpnormal.h"
#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

#include "hud_crosshair.h"

#include "neo_predicted_viewmodel.h"

#include "game_controls/neo_teammenu.h"

#include "ui/neo_hud_compass.h"
#include "ui/neo_hud_game_event.h"
#include "ui/neo_hud_ghost_marker.h"
#include "ui/neo_hud_friendly_marker.h"

#include "ui/neo_hud_elements.h"

#include "baseviewmodel_shared.h"

#include "prediction.h"

#include "neo/weapons/weapon_ghost.h"

#include <engine/ivdebugoverlay.h>
#include <engine/IEngineSound.h>

// Don't alias here
#if defined( CNEO_Player )
#undef CNEO_Player	
#endif

LINK_ENTITY_TO_CLASS(player, C_NEO_Player);

IMPLEMENT_CLIENTCLASS_DT(C_NEO_Player, DT_NEO_Player, CNEO_Player)
	RecvPropInt(RECVINFO(m_iNeoClass)),
	RecvPropInt(RECVINFO(m_iNeoSkin)),

	RecvPropBool(RECVINFO(m_bShowTestMessage)),
	RecvPropString(RECVINFO(m_pszTestMessage)),

	RecvPropInt(RECVINFO(m_iCapTeam)),

	RecvPropVector(RECVINFO(m_vecGhostMarkerPos)),
	RecvPropInt(RECVINFO(m_iGhosterTeam)),
	RecvPropBool(RECVINFO(m_bGhostExists)),
	RecvPropBool(RECVINFO(m_bInThermOpticCamo)),

	RecvPropArray(RecvPropVector(RECVINFO(m_rvFriendlyPlayerPositions[0])), m_rvFriendlyPlayerPositions),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_NEO_Player)
	DEFINE_PRED_FIELD(m_rvFriendlyPlayerPositions, FIELD_VECTOR, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()

ConVar cl_drawhud_quickinfo("cl_drawhud_quickinfo", "0", 0,
	"Whether to display HL2 style ammo/health info near crosshair.",
	true, 0.0f, true, 1.0f);

class NeoLoadoutMenu_Cb : public ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand& command)
	{
		Msg("Loadout access cb\n");

		vgui::EditablePanel *panel = dynamic_cast<vgui::EditablePanel*>
			(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_NEO_LOADOUT));

		if (!panel)
		{
			Assert(false);
			Warning("Couldn't find weapon loadout panel\n");
			return;
		}

		panel->ApplySchemeSettings(vgui::scheme()->GetIScheme(panel->GetScheme()));
		int w, h;
		surface()->GetScreenSize(w, h);
		panel->SetSize(w, h);
		panel->SetPos((int)w / 3, (int)h / 3);

		panel->SetMouseInputEnabled(true);
		panel->SetKeyBoardInputEnabled(true);
		panel->SetCursorAlwaysVisible(true);

		panel->SetControlEnabled("Scout_Button", true);
		panel->SetControlEnabled("Misc2", true);
		panel->SetControlEnabled("Done_Button", true);
		panel->SetControlEnabled("Button1", true);
		panel->SetControlEnabled("Button2", true);
		panel->SetControlEnabled("Button3", true);
		panel->SetControlEnabled("Button4", true);
		panel->SetControlEnabled("Button5", true);
		panel->SetControlEnabled("Button6", true);
		panel->SetControlEnabled("Button7", true);
		panel->SetControlEnabled("Button8", true);
		panel->SetControlEnabled("Button9", true);
		panel->SetControlEnabled("Button10", true);
		panel->SetControlEnabled("Button11", true);
		panel->SetControlEnabled("Button12", true);
		panel->SetControlEnabled("Button13", true);
		panel->SetControlEnabled("Button14", true);

		panel->MoveToFront();

		if (panel->IsKeyBoardInputEnabled())
		{
			panel->RequestFocus();
		}

		panel->SetVisible(true);
		panel->SetEnabled(true);

		surface()->SetMinimized(panel->GetVPanel(), false);
	}
};
NeoLoadoutMenu_Cb neoLoadoutMenu_Cb;

class NeoClassMenu_Cb : public ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand& command)
	{
		Msg("Classmenu access cb\n");

		vgui::EditablePanel *panel = dynamic_cast<vgui::EditablePanel*>
			(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_CLASS));

		if (!panel)
		{
			Assert(false);
			Warning("Couldn't find class panel\n");
			return;
		}

		panel->ApplySchemeSettings(vgui::scheme()->GetIScheme(panel->GetScheme()));
		int w, h;
		surface()->GetScreenSize(w, h);
		panel->SetSize(w, h);
		panel->SetPos((int)w / 3, (int)h / 3);

		panel->SetMouseInputEnabled(true);
		panel->SetKeyBoardInputEnabled(true);
		panel->SetCursorAlwaysVisible(true);

		panel->SetControlEnabled("Scout_Button", true);
		panel->SetControlEnabled("Assault_Button", true);
		panel->SetControlEnabled("Heavy_Button", true);
		panel->SetControlEnabled("Back_Button", true);

		panel->MoveToFront();

		if (panel->IsKeyBoardInputEnabled())
		{
			panel->RequestFocus();
		}

		panel->SetVisible(true);
		panel->SetEnabled(true);

		surface()->SetMinimized(panel->GetVPanel(), false);
	}
};
NeoClassMenu_Cb neoClassMenu_Cb;

class NeoTeamMenu_Cb : public ICommandCallback
{
public:
	virtual void CommandCallback( const CCommand &command )
	{
		Msg("Teammenu access cb\n");

		vgui::EditablePanel *panel = dynamic_cast<vgui::EditablePanel*>
			(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_TEAM));
		if (!panel)
		{
			Assert(false);
			Warning("Couldn't find team panel\n");
			return;
		}

		panel->ApplySchemeSettings(vgui::scheme()->GetIScheme(panel->GetScheme()));
		int w, h;
		surface()->GetScreenSize(w, h);
		panel->SetSize(w, h);
		panel->SetPos((int)w / 3, (int)h / 3);

		panel->SetMouseInputEnabled(true);
		panel->SetKeyBoardInputEnabled(true);
		panel->SetCursorAlwaysVisible(true);

		panel->SetControlEnabled("jinraibutton", true);
		panel->SetControlEnabled("nsfbutton", true);
		panel->SetControlEnabled("specbutton", true);
		panel->SetControlEnabled("autobutton", true);
		panel->SetControlEnabled("CancelButton", true);

		panel->MoveToFront();

		if (panel->IsKeyBoardInputEnabled())
		{
			panel->RequestFocus();
		}

		panel->SetVisible(true);
		panel->SetEnabled(true);

		surface()->SetMinimized(panel->GetVPanel(), false);
	}
};
NeoTeamMenu_Cb neoTeamMenu_Cb;

ConCommand loadoutmenu("loadoutmenu", &neoLoadoutMenu_Cb, "Open weapon loadout selection menu.", FCVAR_USERINFO);
ConCommand classmenu("classmenu", &neoClassMenu_Cb, "Open class selection menu.", FCVAR_USERINFO);
ConCommand teammenu("teammenu", &neoTeamMenu_Cb, "Open team selection menu.", FCVAR_USERINFO);

C_NEO_Player::C_NEO_Player()
{
	m_iNeoClass = NEO_CLASS_ASSAULT;
	m_iNeoSkin = NEO_SKIN_FIRST;

	m_iCapTeam = TEAM_UNASSIGNED;
	m_iGhosterTeam = TEAM_UNASSIGNED;

	m_vecGhostMarkerPos = vec3_origin;
	m_bGhostExists = false;
	m_bShowClassMenu = m_bShowTeamMenu = m_bIsClassMenuOpen = m_bIsTeamMenuOpen = false;
	m_bInThermOpticCamo = m_bUnhandledTocChange = false;

	m_pNeoPanel = NULL;
}

C_NEO_Player::~C_NEO_Player()
{
}

inline void C_NEO_Player::CheckThermOpticButtons()
{
	if (m_afButtonPressed & IN_THERMOPTIC)
	{
		if (IsAlive())
		{
			m_bInThermOpticCamo = !m_bInThermOpticCamo;

			m_bUnhandledTocChange = true;
		}
	}
}

void C_NEO_Player::ZeroFriendlyPlayerLocArray()
{
	for (int i = 0; i < m_rvFriendlyPlayerPositions->Length(); i++)
	{
		m_rvFriendlyPlayerPositions[i].Zero();
	}
}


C_NEO_Player *C_NEO_Player::GetLocalNEOPlayer()
{
	return (C_NEO_Player*)C_BasePlayer::GetLocalPlayer();
}

C_NEOPredictedViewModel *C_NEO_Player::GetNEOViewModel()
{
	return (C_NEOPredictedViewModel*)GetViewModel();
}

int C_NEO_Player::DrawModel( int flags )
{
	if (m_bUnhandledTocChange)
	{
		if (m_bInThermOpticCamo)
		{
			// We should enable camo here, or alternatively call it
			// from the material proxy(?)
		}
		else
		{
			// We should disable camo here
		}

		m_bUnhandledTocChange = false;
	}

	return BaseClass::DrawModel(flags);
}

int C_NEO_Player::GetClass() const
{
	return m_iNeoClass;
}

void C_NEO_Player::AddEntity( void )
{
	BaseClass::AddEntity();
}

ShadowType_t C_NEO_Player::ShadowCastType( void ) 
{
	return BaseClass::ShadowCastType();
}

C_BaseAnimating *C_NEO_Player::BecomeRagdollOnClient()
{
	return BaseClass::BecomeRagdollOnClient();
}

const QAngle& C_NEO_Player::GetRenderAngles()
{
	return BaseClass::GetRenderAngles();
}

bool C_NEO_Player::ShouldDraw( void )
{
	return BaseClass::ShouldDraw();
}

void C_NEO_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged(type);
}

float C_NEO_Player::GetFOV( void )
{
	return BaseClass::GetFOV();
}

CStudioHdr *C_NEO_Player::OnNewModel( void )
{
	return BaseClass::OnNewModel();
}

void C_NEO_Player::TraceAttack( const CTakeDamageInfo &info,
	const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	BaseClass::TraceAttack(info, vecDir, ptr, pAccumulator);
}

void C_NEO_Player::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();

	if (m_afButtonPressed & IN_DROP)
	{
		Weapon_Drop(GetActiveWeapon());
	}

	CheckThermOpticButtons();
}

void C_NEO_Player::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
}

float C_NEO_Player::GetMinFOV() const
{
	return BaseClass::GetMinFOV();
}

Vector C_NEO_Player::GetAutoaimVector( float flDelta )
{
	return BaseClass::GetAutoaimVector(flDelta);
}

void C_NEO_Player::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit(state);
}

void C_NEO_Player::CreateLightEffects(void)
{
	BaseClass::CreateLightEffects();
}

bool C_NEO_Player::ShouldReceiveProjectedTextures( int flags )
{
	return BaseClass::ShouldReceiveProjectedTextures(flags);
}

void C_NEO_Player::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate(updateType);

	CNEOPredictedViewModel *vm = (CNEOPredictedViewModel*)GetViewModel();
	if (vm)
	{
		SetNextThink(CLIENT_THINK_ALWAYS);
	}
}

void C_NEO_Player::PlayStepSound( Vector &vecOrigin,
	surfacedata_t *psurface, float fvol, bool force )
{
	BaseClass::PlayStepSound(vecOrigin, psurface, fvol, force);
}

void C_NEO_Player::PreThink( void )
{
	BaseClass::PreThink();

	CNEOPredictedViewModel *vm = (CNEOPredictedViewModel*)GetViewModel();
	if (vm)
	{
		vm->CalcLean(this);
	}

	if (m_bShowTeamMenu && !m_bIsTeamMenuOpen)
	{
		m_bIsTeamMenuOpen = true;

		engine->ClientCmd(teammenu.GetName());
	}
	else if (m_bShowClassMenu && !m_bIsClassMenuOpen)
	{
		m_bIsClassMenuOpen = true;

		engine->ClientCmd(classmenu.GetName());
	}

	// NEO TODO (Rain): marker should be responsible for its own vis control instead
	CNEOHud_GhostMarker *ghostMarker = NULL;
	if (m_pNeoPanel)
	{
		ghostMarker = m_pNeoPanel->GetGhostMarker();

		if (ghostMarker)
		{
			if (!m_bGhostExists)
			{
				ghostMarker->SetVisible(false);

				//m_pGhostMarker->SetVisible(false);
			}
			else
			{
				const float distance = METERS_PER_INCH *
					GetAbsOrigin().DistTo(m_vecGhostMarkerPos);

				// NEO HACK (Rain): We should test if we're holding a ghost
				// instead of relying on a distance check.
				if (m_iGhosterTeam != GetTeamNumber() || distance > 0.2)
				{
					ghostMarker->SetVisible(true);

					int ghostMarkerX, ghostMarkerY;
					GetVectorInScreenSpace(m_vecGhostMarkerPos, ghostMarkerX, ghostMarkerY);

					ghostMarker->SetScreenPosition(ghostMarkerX, ghostMarkerY);
					ghostMarker->SetGhostingTeam(m_iGhosterTeam);


					ghostMarker->SetGhostDistance(distance);
				}
				else
				{
					ghostMarker->SetVisible(false);
				}
			}
		}
		else
		{
			Warning("Couldn't find ghostMarker\n");
		}

		auto indicator = m_pNeoPanel->GetGameEventIndicator();

		if (indicator)
		{
			if (m_bShowTestMessage)
			{
				indicator->SetMessage(m_pszTestMessage);
			}

			if (indicator->IsVisible() != m_bShowTestMessage)
			{
				indicator->SetVisible(m_bShowTestMessage);
			}
		}
		else
		{
			Warning("Couldn't find GameEventIndicator\n");
		}
	}
}

void C_NEO_Player::ClientThink(void)
{
	BaseClass::ClientThink();
}

void C_NEO_Player::PostThink(void)
{
	BaseClass::PostThink();

	//DevMsg("Roll: %f\n", m_angEyeAngles[2]);

	bool preparingToHideMsg = (m_iCapTeam != TEAM_UNASSIGNED);
	static bool previouslyPreparing = preparingToHideMsg;

	if (!preparingToHideMsg && previouslyPreparing)
	{
		if (m_pNeoPanel && m_pNeoPanel->GetGameEventIndicator())
		{
			m_pNeoPanel->GetGameEventIndicator()->SetVisible(false);
			previouslyPreparing = false;
		}
		else
		{
			Assert(false);
		}
	}

	C_BaseCombatWeapon *pWep = GetActiveWeapon();

	if (pWep)
	{
		static bool previouslyReloading = false;

		if (pWep->m_bInReload)
		{
			if (!previouslyReloading)
			{
				Weapon_SetZoom(false);
			}
		}
		else
		{
			if (m_afButtonReleased & IN_AIM)
			{
				Weapon_AimToggle(pWep);
			}
		}

		previouslyReloading = pWep->m_bInReload;
	}

	if (m_iNeoClass == NEO_CLASS_RECON)
	{
		if ((m_afButtonPressed & IN_JUMP) && (m_nButtons & IN_SPEED))
		{
			const float superJumpCost = 45.0f;
			// The suit check is for prediction only, actual power drain happens serverside
			if (!IsCarryingGhost() && !GetMoveParent() &&
				m_HL2Local.m_flSuitPower >= superJumpCost)
			{
				SuperJump();
			}
		}
	}
}

// This is applied for prediction purposes. It should match CNEO_Player's method.
inline void C_NEO_Player::SuperJump(void)
{
	Vector forward;
	AngleVectors(EyeAngles(), &forward);

	// We don't give an upwards boost aside from regular jump
	forward.z = 0;

	ApplyAbsVelocityImpulse(forward * neo_recon_superjump_intensity.GetFloat());
}

void C_NEO_Player::Spawn( void )
{
	BaseClass::Spawn();

	if (GetTeamNumber() == TEAM_UNASSIGNED)
	{
		m_bShowTeamMenu = true;
	}

	// NEO TODO (Rain): UI elements should do this themselves
	if (!m_pNeoPanel)
	{
		m_pNeoPanel = dynamic_cast<CNeoHudElements*>
			(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_NEO_HUD));

		if (!m_pNeoPanel)
		{
			Assert(false);
			Warning("Couldn't find CNeoHudElements panel\n");
			return;
		}

		m_pNeoPanel->ShowPanel(true);

		auto compass = m_pNeoPanel->GetCompass();
		if (compass)
		{
			compass->SetOwner(this);
		}
		else
		{
			Assert(false);
			Warning("Couldn't find compass HUD element\n");
		}

		auto iff = m_pNeoPanel->GetIFF();
		if (iff)
		{
			iff->SetOwner(this);
		}
		else
		{
			Assert(false);
			Warning("Couldn't find compass IFF element\n");
		}
	}

#if(0)
	// We could support crosshair customization/colors etc this way.
	auto cross = GET_HUDELEMENT(CHudCrosshair);
	Color color = Color(255, 255, 255, 255);
	cross->SetCrosshair(NULL, color);
#endif
}

void C_NEO_Player::DoImpactEffect( trace_t &tr, int nDamageType )
{
	BaseClass::DoImpactEffect(tr, nDamageType);
}

IRagdoll* C_NEO_Player::GetRepresentativeRagdoll() const
{
	return BaseClass::GetRepresentativeRagdoll();
}

void C_NEO_Player::CalcView( Vector &eyeOrigin, QAngle &eyeAngles,
	float &zNear, float &zFar, float &fov )
{
	BaseClass::CalcView(eyeOrigin, eyeAngles, zNear, zFar, fov);
}

const QAngle &C_NEO_Player::EyeAngles()
{
	return BaseClass::EyeAngles();
}

// Whether to draw the HL2 style quick health/ammo info around the crosshair.
// Clients can control their preference with a ConVar that gets polled here.
bool C_NEO_Player::ShouldDrawHL2StyleQuickHud(void)
{
	return cl_drawhud_quickinfo.GetBool();
}

void C_NEO_Player::Weapon_Drop(C_BaseCombatWeapon *pWeapon)
{
	C_WeaponGhost *ghost = dynamic_cast<C_WeaponGhost*>(pWeapon);
	if (ghost)
	{
		ghost->HandleGhostUnequip();
	}
}

void C_NEO_Player::StartSprinting(void)
{
	if (m_HL2Local.m_flSuitPower < 10)
	{
		return;
	}

	if (IsCarryingGhost())
	{
		return;
	}

	SetMaxSpeed(GetSprintSpeed());
}

void C_NEO_Player::StopSprinting(void)
{
	SetMaxSpeed(GetNormSpeed());

	m_fIsSprinting = false;
}

bool C_NEO_Player::CanSprint(void)
{
	if (m_iNeoClass == NEO_CLASS_SUPPORT)
	{
		return false;
	}

	return BaseClass::CanSprint();
}

void C_NEO_Player::StartWalking(void)
{
	SetMaxSpeed(GetWalkSpeed());

	m_fIsWalking = true;
}

void C_NEO_Player::StopWalking(void)
{
	SetMaxSpeed(GetNormSpeed());

	m_fIsWalking = true;
}

float C_NEO_Player::GetCrouchSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_CROUCH_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_CROUCH_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_CROUCH_SPEED;
	}

	return NEO_BASE_CROUCH_SPEED;
}

float C_NEO_Player::GetNormSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_NORM_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_NORM_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_NORM_SPEED;
	}

	return NEO_BASE_NORM_SPEED;
}

float C_NEO_Player::GetWalkSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_WALK_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_WALK_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_WALK_SPEED;
	}

	return NEO_BASE_WALK_SPEED;
}

float C_NEO_Player::GetSprintSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_SPRINT_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_SPRINT_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_SPRINT_SPEED;
	}

	return NEO_BASE_SPRINT_SPEED;
}

void C_NEO_Player::Weapon_AimToggle(C_BaseCombatWeapon *pWep)
{
	// NEO TODO/HACK: Not all neo weapons currently inherit
	// through a base neo class, so we can't static_cast!!
	auto neoCombatWep = dynamic_cast<C_NEOBaseCombatWeapon*>(pWep);
	if (!neoCombatWep)
	{
		return;
	}
	else if (!IsAllowedToZoom(neoCombatWep))
	{
		return;
	}

	bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) == HIDEHUD_CROSSHAIR;
	Weapon_SetZoom(showCrosshair);
}

inline void C_NEO_Player::Weapon_SetZoom(bool bZoomIn)
{
	const float zoomSpeedSecs = 0.25f;

	const int zoomAmount = 30;

	if (bZoomIn)
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;

		SetFOV((CBaseEntity*)this, GetDefaultFOV() - zoomAmount, zoomSpeedSecs);
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;

		SetFOV((CBaseEntity*)this, GetDefaultFOV(), zoomSpeedSecs);
	}
}

inline bool C_NEO_Player::IsCarryingGhost(void)
{
#ifdef DEBUG
	auto baseWep = GetWeapon(NEO_WEAPON_PRIMARY_SLOT);
	if (!baseWep)
	{
		return false;
	}

	auto wep = dynamic_cast<CNEOBaseCombatWeapon*>(baseWep);
	if (!wep)
	{
		Assert(false);
	}
#else
	auto wep = static_cast<CNEOBaseCombatWeapon*>(GetWeapon(NEO_WEAPON_PRIMARY_SLOT));
#endif
	return (wep && wep->IsGhost());
}
