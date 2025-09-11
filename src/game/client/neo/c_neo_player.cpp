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

#include "iinput.h"
#include "in_main.h"

#include "clientmode_hl2mpnormal.h"
#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

#include "hud_crosshair.h"

#include "neo_predicted_viewmodel.h"

#include "game_controls/neo_teammenu.h"
#include "game_controls/neo_classmenu.h"
#include "game_controls/neo_loadoutmenu.h"

#include "ui/neo_hud_compass.h"
#include "ui/neo_hud_game_event.h"
#include "ui/neo_hud_ghost_marker.h"
#include "ui/neo_hud_friendly_marker.h"
#include "ui/neo_hud_round_state.h"

#include "neo/game_controls/neo_loadoutmenu.h"

#include "baseviewmodel_shared.h"

#include "prediction.h"

#include "neo/weapons/weapon_ghost.h"
#include "neo/weapons/weapon_supa7.h"

#include <engine/ivdebugoverlay.h>
#include <engine/IEngineSound.h>

#include <materialsystem/imaterialsystem.h>
#include <materialsystem/itexture.h>
#include "rendertexture.h"
#include "ivieweffects.h"
#include "c_neo_killer_damage_infos.h"
#include <vgui/ILocalize.h>
#include <tier3.h>

#include "model_types.h"

#include "c_user_message_register.h"
#include "neo_player_shared.h"

// Don't alias here
#if defined( CNEO_Player )
#undef CNEO_Player	
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(player, C_NEO_Player);

IMPLEMENT_CLIENTCLASS_DT(C_NEO_Player, DT_NEO_Player, CNEO_Player)
	RecvPropInt(RECVINFO(m_iNeoClass)),
	RecvPropInt(RECVINFO(m_iNeoSkin)),
	RecvPropInt(RECVINFO(m_iNeoStar)),

	RecvPropBool(RECVINFO(m_bShowTestMessage)),
	RecvPropString(RECVINFO(m_pszTestMessage)),

	RecvPropInt(RECVINFO(m_iXP)),
	RecvPropInt(RECVINFO(m_iLoadoutWepChoice)),
	RecvPropInt(RECVINFO(m_iNextSpawnClassChoice)),
	RecvPropInt(RECVINFO(m_bInLean)),
	RecvPropEHandle(RECVINFO(m_hDroppedJuggernautItem)),

	RecvPropBool(RECVINFO(m_bInThermOpticCamo)),
	RecvPropBool(RECVINFO(m_bLastTickInThermOpticCamo)),
	RecvPropBool(RECVINFO(m_bInVision)),
	RecvPropBool(RECVINFO(m_bHasBeenAirborneForTooLongToSuperJump)),
	RecvPropBool(RECVINFO(m_bInAim)),
	RecvPropBool(RECVINFO(m_bIneligibleForLoadoutPick)),
	RecvPropBool(RECVINFO(m_bCarryingGhost)),

	RecvPropTime(RECVINFO(m_flCamoAuxLastTime)),
	RecvPropInt(RECVINFO(m_nVisionLastTick)),
	RecvPropTime(RECVINFO(m_flJumpLastTime)),
	RecvPropTime(RECVINFO(m_flNextPingTime)),

	RecvPropArray(RecvPropInt(RECVINFO(m_rfAttackersScores[0])), m_rfAttackersScores),
	RecvPropArray(RecvPropFloat(RECVINFO(m_rfAttackersAccumlator[0])), m_rfAttackersAccumlator),
	RecvPropArray(RecvPropInt(RECVINFO(m_rfAttackersHits[0])), m_rfAttackersHits),

	RecvPropInt(RECVINFO(m_NeoFlags)),
	RecvPropString(RECVINFO(m_szNeoName)),
	RecvPropString(RECVINFO(m_szNeoClantag)),
	RecvPropString(RECVINFO(m_szNeoCrosshair)),
	RecvPropInt(RECVINFO(m_szNameDupePos)),
	RecvPropBool(RECVINFO(m_bClientWantNeoName)),

	RecvPropTime(RECVINFO(m_flDeathTime)),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_NEO_Player)
	DEFINE_PRED_ARRAY(m_rfAttackersScores, FIELD_INTEGER, MAX_PLAYERS, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_ARRAY(m_rfAttackersAccumlator, FIELD_FLOAT, MAX_PLAYERS, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_ARRAY(m_rfAttackersHits, FIELD_INTEGER, MAX_PLAYERS, FTYPEDESC_INSENDTABLE),

	DEFINE_PRED_FIELD_TOL(m_flCamoAuxLastTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
	
	DEFINE_PRED_FIELD(m_bInThermOpticCamo, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bLastTickInThermOpticCamo, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bInAim, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bInLean, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bInVision, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bHasBeenAirborneForTooLongToSuperJump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),

	DEFINE_PRED_FIELD(m_nVisionLastTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD_TOL(m_flJumpLastTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
	DEFINE_PRED_FIELD_TOL(m_flNextPingTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
END_PREDICTION_DATA()

static void __MsgFunc_IdleRespawnShowMenu(bf_read &)
{
	if (C_NEO_Player::GetLocalNEOPlayer())
	{
		engine->ClientCmd("classmenu");
	}
}
USER_MESSAGE_REGISTER(IdleRespawnShowMenu);

ConVar cl_drawhud_quickinfo("cl_drawhud_quickinfo", "0", 0,
	"Whether to display HL2 style ammo/health info near crosshair.",
	true, 0.0f, true, 1.0f);

ConVar cl_neo_streamermode("cl_neo_streamermode", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Streamer mode turns player names into generic names and hide avatars.", true, 0.0f, true, 1.0f);
ConVar cl_neo_streamermode_autodetect_obs("cl_neo_streamermode_autodetect_obs", "0", FCVAR_ARCHIVE, "Automatically turn cl_neo_streamermode on if OBS was detected on startup.", true, 0.0f, true, 1.0f);

extern ConVar sv_neo_clantag_allow;
extern ConVar sv_neo_dev_test_clantag;

class NeoLoadoutMenu_Cb : public ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand& command)
	{
		if (engine->IsPlayingDemo() || NEORules()->GetForcedWeapon() >= 0)
		{
			return;
		}

		auto team = GetLocalPlayerTeam();
		if(team < FIRST_GAME_TEAM)
		{
			return;
		}

		auto panel = assert_cast<vgui::EditablePanel*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_NEO_LOADOUT));

		if (!panel)
		{
			Assert(false);
			Warning("Couldn't find weapon loadout panel\n");
			return;
		}

		auto classPanel = assert_cast<CNeoClassMenu*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_CLASS));
		if (classPanel)
		{
			classPanel->ShowPanel(false);
		}
		auto teamPanel = assert_cast<CNeoTeamMenu*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_TEAM));
		if (teamPanel)
		{
			teamPanel->ShowPanel(false);
		}

		if (panel->IsVisible() && panel->IsEnabled())
		{
			panel->MoveToFront();
			return;	// Prevent cursor stuck
		}

		panel->SetProportional(false); // Fixes wrong menu size when in windowed mode, regardless of whether proportional is set to false in the res file (NEOWTF)
		panel->ApplySchemeSettings(vgui::scheme()->GetIScheme(panel->GetScheme()));

		panel->SetMouseInputEnabled(true);
		//panel->SetKeyBoardInputEnabled(true);
		panel->SetCursorAlwaysVisible(true);

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
		panel->SetControlEnabled("ReturnButton", true);

		panel->MoveToFront();

		if (panel->IsKeyBoardInputEnabled())
		{
			panel->RequestFocus();
		}

		panel->SetVisible(true);
		panel->SetEnabled(true);

		auto loadoutPanel = assert_cast<CNeoLoadoutMenu*>(panel);
		if (loadoutPanel)
		{
			loadoutPanel->ShowPanel(true);
		}
		else
		{
			Assert(false);
			DevWarning("Cast failed\n");
		}

		surface()->SetMinimized(panel->GetVPanel(), false);
	}
};
NeoLoadoutMenu_Cb neoLoadoutMenu_Cb;

class NeoClassMenu_Cb : public ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand& command)
	{
		if (engine->IsPlayingDemo() || NEORules()->GetForcedClass() >= 0)
		{
			return;
		}

		if (!command.FindArg("skipTeamCheck"))
		{ // A smarter way to do this might be to assign the buttons for class and weapons menu to unique commands that check the current team and call this commandcallback
			auto team = GetLocalPlayerTeam();
			if(team < FIRST_GAME_TEAM)
			{
				return;
			}
		}

		C_NEO_Player* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
		if (!localPlayer)
		{
			return;
		}

		auto playerNeoClass = localPlayer->m_iNeoClass;
		if (playerNeoClass == NEO_CLASS_VIP)
		{
			return;
		}
		
		vgui::EditablePanel *panel = assert_cast<vgui::EditablePanel*>
			(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_CLASS));

		if (!panel)
		{
			Assert(false);
			Warning("Couldn't find class panel\n");
			return;
		}

		auto loadoutPanel = assert_cast<CNeoLoadoutMenu*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_NEO_LOADOUT));
		if (loadoutPanel)
		{
			loadoutPanel->ShowPanel(false);
		}
		auto teamPanel = assert_cast<CNeoTeamMenu*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_TEAM));
		if (teamPanel)
		{
			teamPanel->ShowPanel(false);
		}

		if (panel->IsVisible() && panel->IsEnabled())
		{
			panel->MoveToFront();
			return;	// Prevent cursor stuck
		}

		panel->SetProportional(false);
		panel->ApplySchemeSettings(vgui::scheme()->GetIScheme(panel->GetScheme()));

		panel->SetMouseInputEnabled(true);
		//panel->SetKeyBoardInputEnabled(true);
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
		if (engine->IsPlayingDemo() || NEORules()->GetForcedTeam() >= 0)
		{
			return;
		}

		if (!g_pNeoTeamMenu)
		{
			Assert(false);
			DevWarning("CNeoTeamMenu is not ready\n");
			return;
		}

		vgui::EditablePanel *panel = assert_cast<vgui::EditablePanel*>
			(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_TEAM));
		if (!panel)
		{
			Assert(false);
			Warning("Couldn't find team panel\n");
			return;
		}

		auto loadoutPanel = assert_cast<CNeoLoadoutMenu*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_NEO_LOADOUT));
		if (loadoutPanel)
		{
			loadoutPanel->ShowPanel(false);
		}
		auto classPanel = assert_cast<CNeoClassMenu*>(GetClientModeNormal()->
			GetViewport()->FindChildByName(PANEL_CLASS));
		if (classPanel)
		{
			classPanel->ShowPanel(false);
		}

		if (panel->IsVisible() && panel->IsEnabled())
		{
			panel->MoveToFront();
			return;	// Prevent cursor stuck
		}

		panel->SetProportional(false);
		panel->ApplySchemeSettings(vgui::scheme()->GetIScheme(panel->GetScheme()));

		panel->SetMouseInputEnabled(true);
		//panel->SetKeyBoardInputEnabled(true);
		panel->SetCursorAlwaysVisible(true);

		panel->SetControlEnabled("jinraibutton", true);
		panel->SetControlEnabled("nsfbutton", true);
		panel->SetControlEnabled("specbutton", true);
		panel->SetControlEnabled("autobutton", true);
		panel->SetControlEnabled("CancelButton", true);

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

class VguiCancel_Cb : public ICommandCallback
{
public:
	virtual void CommandCallback(const CCommand& command)
	{
		auto player = C_NEO_Player::GetLocalNEOPlayer();
		Assert(player);
		if (player)
		{
			if (player->GetTeamNumber() <= TEAM_UNASSIGNED)
			{
				engine->ClientCmd("jointeam 0");
			}
		}
	}
};
VguiCancel_Cb vguiCancel_Cb;

ConCommand loadoutmenu("loadoutmenu", &neoLoadoutMenu_Cb, "Open weapon loadout selection menu.", FCVAR_USERINFO | FCVAR_DONTRECORD);
ConCommand classmenu("classmenu", &neoClassMenu_Cb, "Open class selection menu.", FCVAR_USERINFO | FCVAR_DONTRECORD);
ConCommand teammenu("teammenu", &neoTeamMenu_Cb, "Open team selection menu.", FCVAR_USERINFO | FCVAR_DONTRECORD);
ConCommand vguicancel("vguicancel", &vguiCancel_Cb, "Cancel current vgui screen.", FCVAR_USERINFO | FCVAR_DONTRECORD);

C_NEO_Player::C_NEO_Player()
{
	SetPredictionEligible(true);

	m_iNeoClass = NEORules()->GetForcedClass() >= 0 ? NEORules()->GetForcedClass() : NEO_CLASS_ASSAULT;
	m_iNeoSkin = NEORules()->GetForcedSkin() >= 0 ? NEORules()->GetForcedSkin() : NEO_SKIN_FIRST;
	m_iNeoStar = NEO_DEFAULT_STAR;
	V_memset(m_szNeoName.GetForModify(), 0, sizeof(m_szNeoName));
	V_memset(m_szNeoClantag.GetForModify(), 0, sizeof(m_szNeoClantag));
	V_memset(m_szNeoCrosshair.GetForModify(), 0, sizeof(m_szNeoCrosshair));

	m_iLoadoutWepChoice = NEORules()->GetForcedWeapon() >= 0 ? NEORules()->GetForcedWeapon() : 0;
	m_iNextSpawnClassChoice = -1;
	m_iXP.GetForModify() = 0;

	m_bInThermOpticCamo = m_bInVision = false;
	m_bHasBeenAirborneForTooLongToSuperJump = false;
	m_bInAim = false;
	m_bCarryingGhost = false;
	m_bIneligibleForLoadoutPick = false;
	m_bInLean = NEO_LEAN_NONE;

	m_flCamoAuxLastTime = 0;
	m_nVisionLastTick = 0;
	m_flLastAirborneJumpOkTime = 0;
	m_flLastSuperJumpTime = 0;

	m_bFirstAliveTick = true;
	m_bFirstDeathTick = true;
	m_bPreviouslyReloading = false;
	m_bLastTickInThermOpticCamo = false;
	m_bIsAllowedToToggleVision = false;

	m_flTocFactor = 0.15f;

	memset(m_szNeoNameWDupeIdx, 0, sizeof(m_szNeoNameWDupeIdx));
	m_szNameDupePos = 0;
}

C_NEO_Player::~C_NEO_Player()
{
#ifdef GLOWS_ENABLE
	DestroyGlowEffect();
#endif // GLOWS_ENABLE
}

void C_NEO_Player::CheckThermOpticButtons()
{
	m_bLastTickInThermOpticCamo = m_bInThermOpticCamo;

	if ((m_afButtonPressed & IN_THERMOPTIC) && IsAlive())
	{
		if (GetClass() == NEO_CLASS_SUPPORT)
		{
			return;
		}

		if (!m_bInThermOpticCamo && m_HL2Local.m_cloakPower >= MIN_CLOAK_AUX)
		{
			SetCloakState( true );
		}
		else
		{
			SetCloakState( false );
		}
	}

	if (m_bInThermOpticCamo != m_bLastTickInThermOpticCamo)
	{
		PlayCloakSound();
	}
}

void C_NEO_Player::CheckVisionButtons()
{
	if (!m_bIsAllowedToToggleVision)
	{
		return;
	}

	if (m_afButtonPressed & IN_VISION)
	{
		if (IsAlive())
		{
			m_bIsAllowedToToggleVision = false;

			m_bInVision = !m_bInVision;

			if (m_bInVision)
			{
				DevMsg("Playing sound at :%f\n", gpGlobals->curtime);

				C_RecipientFilter filter;
				filter.AddRecipient(this);
				filter.MakeReliable();

				EmitSound_t params;
				params.m_bEmitCloseCaption = false;
				params.m_pOrigin = &GetAbsOrigin();
				params.m_nChannel = CHAN_ITEM;
				params.m_nFlags |= SND_DO_NOT_OVERWRITE_EXISTING_ON_CHANNEL;
				static int visionToggle = CBaseEntity::PrecacheScriptSound("NeoPlayer.VisionOn");
				params.m_hSoundScriptHandle = visionToggle;

				EmitSound(filter, entindex(), params);
			}
		}
	}
}

void C_NEO_Player::CheckLeanButtons()
{
	if (!IsAlive())
	{
		return;
	}

	m_bInLean = NEO_LEAN_NONE;
	if ((m_nButtons & IN_LEAN_LEFT) && !(m_nButtons & IN_LEAN_RIGHT || IsSprinting()))
	{
		m_bInLean = NEO_LEAN_LEFT;
	}
	else if ((m_nButtons & IN_LEAN_RIGHT) && !(m_nButtons & IN_LEAN_LEFT || IsSprinting()))
	{
		m_bInLean = NEO_LEAN_RIGHT;
	}
}

int C_NEO_Player::GetAttackersScores(const int attackerIdx) const
{
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_DM || NEORules()->GetGameType() == NEO_GAME_TYPE_TDM)
	{
		return m_rfAttackersScores.Get(attackerIdx);
	}
	return min(m_rfAttackersScores.Get(attackerIdx), 100);
}

const char *C_NEO_Player::GetNeoClantag() const
{
	if (!sv_neo_clantag_allow.GetBool() ||
			(cl_neo_streamermode.GetBool() && !IsLocalPlayer()))
	{
		return "";
	}
#ifdef DEBUG
	const char *overrideClantag = sv_neo_dev_test_clantag.GetString();
	if (overrideClantag && overrideClantag[0])
	{
		return overrideClantag;
	}
#endif
	return m_szNeoClantag.Get();
}

const char *C_NEO_Player::InternalGetNeoPlayerName() const
{
	const int dupePos = m_szNameDupePos;
	const bool localWantNeoName = GetLocalNEOPlayer()->ClientWantNeoName();
	if (localWantNeoName && m_szNeoName.Get()[0] != '\0')
	{
		const char *neoName = m_szNeoName.Get();
		if (dupePos > 0)
		{
			if (dupePos != m_szNeoNameLocalDupeIdx)
			{
				m_szNeoNameLocalDupeIdx = dupePos;
				V_sprintf_safe(m_szNeoNameWDupeIdx, "%s (%d)", neoName, dupePos);
			}
			return m_szNeoNameWDupeIdx;
		}
		return neoName;
	}

	const char *stndName = const_cast<C_NEO_Player *>(this)->GetPlayerName();
	if (localWantNeoName && dupePos > 0)
	{
		if (dupePos != m_szNeoNameLocalDupeIdx)
		{
			m_szNeoNameLocalDupeIdx = dupePos;
			V_sprintf_safe(m_szNeoNameWDupeIdx, "%s (%d)", stndName, dupePos);
		}
		return m_szNeoNameWDupeIdx;
	}
	return stndName;
}

const char *C_NEO_Player::GetNeoPlayerName() const
{
	if (cl_neo_streamermode.GetBool() && !IsLocalPlayer())
	{
		[[maybe_unused]] uchar32 u32Out;
		bool bError = false;
		const char *pSzName = InternalGetNeoPlayerName();
		const int iSize = Q_UTF8ToUChar32(pSzName, u32Out, bError);
		if (!bError) V_memcpy(gStreamerModeNames[entindex()], pSzName, iSize);
		return gStreamerModeNames[entindex()];
	}
	return InternalGetNeoPlayerName();
}

bool C_NEO_Player::ClientWantNeoName() const
{
	return m_bClientWantNeoName;
}

int C_NEO_Player::GetAttackerHits(const int attackerIdx) const
{
	return m_rfAttackersHits.Get(attackerIdx);
}

constexpr float invertedDamageResistanceModifier[NEO_CLASS__ENUM_COUNT] = {
	1 / NEO_RECON_DAMAGE_MODIFIER,
	1 / NEO_ASSAULT_DAMAGE_MODIFIER,
	1 / NEO_SUPPORT_DAMAGE_MODIFIER,
	1 / NEO_ASSAULT_DAMAGE_MODIFIER
};

int C_NEO_Player::GetDisplayedHealth(bool asPercent) const
{
	return asPercent ? GetHealth() : GetHealth() * invertedDamageResistanceModifier[m_iNeoClass];
}

extern ConVar mat_neo_toc_test;
#ifdef GLOWS_ENABLE
extern ConVar glow_outline_effect_enable;
#endif // GLOWS_ENABLE
int C_NEO_Player::DrawModel(int flags)
{
	if (flags & STUDIO_IGNORE_NEO_EFFECTS || !(flags & STUDIO_RENDER))
	{
		return BaseClass::DrawModel(flags);
	}

#ifdef GLOWS_ENABLE
	auto pTargetPlayer = glow_outline_effect_enable.GetBool() ? C_NEO_Player::GetLocalNEOPlayer() : C_NEO_Player::GetVisionTargetNEOPlayer();
#else
	auto pTargetPlayer = C_NEO_Player::GetVisionTargetNEOPlayer();
#endif // GLOWS_ENABLE
	if (!pTargetPlayer)
	{
		Assert(false);
		return BaseClass::DrawModel(flags);
	}
	
	bool inThermalVision = pTargetPlayer ? (pTargetPlayer->IsInVision() && pTargetPlayer->GetClass() == NEO_CLASS_SUPPORT) : false;

	int ret = 0;
	if (!IsCloaked() || inThermalVision)
	{
		ret |= BaseClass::DrawModel(flags);
	}

	if (IsCloaked() && !inThermalVision)
	{
		mat_neo_toc_test.SetValue(m_flTocFactor);
		IMaterial* pass = materials->FindMaterial("models/player/toc", TEXTURE_GROUP_CLIENT_EFFECTS);
		modelrender->ForcedMaterialOverride(pass);
		ret |= BaseClass::DrawModel(flags);
		modelrender->ForcedMaterialOverride(nullptr);
	}

	else if (inThermalVision && !IsCloaked())
	{
		IMaterial* pass = materials->FindMaterial("dev/thermal_model", TEXTURE_GROUP_MODEL);
		modelrender->ForcedMaterialOverride(pass);
		ret |= BaseClass::DrawModel(flags);
		modelrender->ForcedMaterialOverride(nullptr);
	}

	return ret;
}

void C_NEO_Player::AddEntity( void )
{
	BaseClass::AddEntity();
}

void C_NEO_Player::AddPoints(int score, bool bAllowNegativeScore)
{
	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (m_iXP < 0)		// Can't go more negative
				return;

			if (-score > m_iXP)	// Will this go negative?
			{
				score = -m_iXP;		// Sum will be 0
			}
		}
	}

	m_iXP += score;
	//pl.frags = m_iFrags; NEO TODO (Adam) Is this actually used anywhere? should we include a xp field in CPlayerState?
}

ShadowType_t C_NEO_Player::ShadowCastType( void ) 
{
	if (IsCloaked())
	{
		return SHADOWS_NONE;
	}
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

RenderGroup_t C_NEO_Player::GetRenderGroup()
{
	return IsCloaked() ? RENDER_GROUP_TRANSLUCENT_ENTITY : RENDER_GROUP_OPAQUE_ENTITY;
}

bool C_NEO_Player::UsesPowerOfTwoFrameBufferTexture()
{
	return IsCloaked();
}

bool C_NEO_Player::ShouldDraw( void )
{
	return BaseClass::ShouldDraw();
}

void C_NEO_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged(type);

	// Had to delay client sync until player model was ready
	CSpectatorTakeoverPlayerUpdateOnDataChanged();
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
		if (auto neoWep = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon()))
		{
			Weapon_Drop(neoWep);
		}
	}
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

extern ConVar neo_ghost_bhopping;
void C_NEO_Player::CalculateSpeed(void)
{
	float speed = GetNormSpeed();

	if (auto pNeoWep = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon()))
	{
		speed *= pNeoWep->GetSpeedScale();
	}

	if (GetFlags() & FL_DUCKING)
	{
		speed *= NEO_CROUCH_MODIFIER;
	}

	if (IsSprinting())
	{
		switch (m_iNeoClass) {
			case NEO_CLASS_RECON:
				speed *= NEO_RECON_SPRINT_MODIFIER;
				break;
			case NEO_CLASS_ASSAULT:
			case NEO_CLASS_VIP:
			case NEO_CLASS_JUGGERNAUT:
				speed *= NEO_ASSAULT_SPRINT_MODIFIER;
				break;
			case NEO_CLASS_SUPPORT:
				speed *= NEO_SUPPORT_SPRINT_MODIFIER; // Should never happen
				break;
			default:
				break;
		}
	}

	if (IsInAim())
	{
		speed *= NEO_AIM_MODIFIER;
	}

	if (m_nButtons & IN_WALK)
	{
		speed = MIN(GetFlags() & FL_DUCKING ? NEO_CROUCH_WALK_SPEED : NEO_WALK_SPEED, speed);
	}

	Vector absoluteVelocity = GetAbsVelocity();
	absoluteVelocity.z = 0.f;
	float currentSpeed = absoluteVelocity.Length();

	if (((!neo_ghost_bhopping.GetBool() && m_bCarryingGhost) || m_iNeoClass == NEO_CLASS_JUGGERNAUT) && GetMoveType() == MOVETYPE_WALK && currentSpeed > speed)
	{
		float overSpeed = currentSpeed - speed;
		absoluteVelocity.NormalizeInPlace();
		absoluteVelocity *= -overSpeed;
		ApplyAbsVelocityImpulse(absoluteVelocity);
	}
	speed = MAX(speed, 55);

	// Slowdown after jumping
	if (m_iNeoClass != NEO_CLASS_RECON)
	{
		const float timeSinceJumping = gpGlobals->curtime - m_flJumpLastTime;
		constexpr float SLOWDOWN_TIME = 1.15f;
		if (timeSinceJumping < SLOWDOWN_TIME)
		{
			speed = MAX(75, speed * (1 - ((SLOWDOWN_TIME - timeSinceJumping) * 1.75)));
		}
	}
	SetMaxSpeed(speed);
}

void C_NEO_Player::HandleSpeedChangesLegacy()
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	if( buttonsChanged & (IN_SPEED | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)  || m_nButtons & IN_SPEED)
	{
		// The state of the sprint/run button has changed.
		if ( IsSuitEquipped() )
		{
#ifdef NEO
			constexpr float MOVING_SPEED_MINIMUM = 0.5f; // NEOTODO (Adam) This is the same value as defined in cbaseanimating, should we be using the same value? Should we import it here?
			if (!(m_nButtons & IN_SPEED) && IsSprinting())
#else
			if ( !(m_nButtons & (IN_SPEED | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)) && IsSprinting())
#endif
			{
				StopSprinting();
			}
#ifdef NEO
			else if ((m_nButtons & IN_SPEED) && !IsSprinting() && GetLocalVelocity().IsLengthGreaterThan(MOVING_SPEED_MINIMUM))
#else
			else if ( (m_afButtonPressed & IN_SPEED) && (m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)) && !IsSprinting() )
#endif
			{
				if ( CanSprint() )
				{
					StartSprinting();
				}
				else
				{
					// Reset key, so it will be activated post whatever is suppressing it.
					m_nButtons &= ~IN_SPEED;
				}
			}
		}
	}
	else if( buttonsChanged & IN_WALK )
	{
		if ( IsSuitEquipped() )
		{
			// The state of the WALK button has changed.
			if( IsWalking() && !(m_afButtonPressed & IN_WALK) )
			{
				StopWalking();
			}
			else if( !IsWalking() && !IsSprinting() && (m_afButtonPressed & IN_WALK) )
			{
				StartWalking();
			}
		}
	}

	if ( IsSuitEquipped() && m_fIsWalking && !(m_nButtons & IN_WALK)  )
		StopWalking();
}

#if 0
void C_NEO_Player::HandleSpeedChanges( CMoveData *mv )
{
	int nChangedButtons = mv->m_nButtons ^ mv->m_nOldButtons;

	bool bJustPressedSpeed = !!( nChangedButtons & IN_SPEED );

#ifdef NEO
	constexpr float MOVING_SPEED_MINIMUM = 0.5f; // NEOTODO (Adam) This is the same value as defined in cbaseanimating, should we be using the same value? Should we import it here?
	const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && ( mv->m_nButtons & IN_SPEED ) && GetLocalVelocity().IsLengthGreaterThan(MOVING_SPEED_MINIMUM));
#else
	const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && ( mv->m_nButtons & IN_SPEED ) );
#endif

#ifdef NEO
	const bool bWantsToChangeSprinting = ( IsSprinting() != bWantSprint ) && ((mv->m_nButtons & IN_SPEED) || (( nChangedButtons & IN_SPEED ) != 0));
#else
	const bool bWantsToChangeSprinting = ( m_HL2Local.m_bNewSprinting != bWantSprint ) && ( nChangedButtons & IN_SPEED ) != 0;
#endif

	bool bSprinting = IsSprinting();
	if ( bWantsToChangeSprinting )
	{
		if ( bWantSprint )
		{
#ifdef NEO
			if ( m_HL2Local.m_flSuitPower < SPRINT_START_MIN )
#else
			if ( m_HL2Local.m_flSuitPower < 10.0f )
#endif
			{
#ifndef NEO
				if ( bJustPressedSpeed )
				{
					CPASAttenuationFilter filter( this );
					filter.UsePredictionRules();
					EmitSound( filter, entindex(), "HL2Player.SprintNoPower" );
				}
#endif
			}
			else
			{
				bSprinting = true;
			}
		}
		else
		{
			bSprinting = false;
		}
	}

	if ( m_HL2Local.m_flSuitPower < 0.01 )
	{
		bSprinting = false;
	}

	bool bWantWalking;

	if ( IsSuitEquipped() )
	{
		bWantWalking = ( mv->m_nButtons & IN_WALK ) && !bSprinting && !( mv->m_nButtons & IN_DUCK );
	}
	else
	{
		bWantWalking = true;
	}

	if ( bWantWalking )
	{
		bSprinting = false;
	}

	m_HL2Local.m_bNewSprinting = bSprinting;

	if ( bSprinting )
	{
#ifndef NEO
		if ( bJustPressedSpeed )
		{
			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "HL2Player.SprintStart" );
		}
#endif
		mv->m_flClientMaxSpeed = GetSprintSpeed_WithActiveWepEncumberment();
	}
	else if ( bWantWalking )
	{
		mv->m_flClientMaxSpeed = GetCrouchSpeed_WithActiveWepEncumberment();
	}
	else
	{
		mv->m_flClientMaxSpeed = GetNormSpeed_WithActiveWepEncumberment();
	}

	mv->m_flMaxSpeed = MaxSpeed();
}
#endif

extern ConVar sv_infinite_aux_power;
extern ConVar glow_outline_effect_enable;
void C_NEO_Player::PreThink( void )
{
	BaseClass::PreThink();

	HandleSpeedChangesLegacy();

	if (m_HL2Local.m_flSuitPower <= 0.0f && IsSprinting())
	{
		StopSprinting();
	}

	CalculateSpeed();

	CheckThermOpticButtons();
	CheckVisionButtons();
	CheckPingButton(this);

	if (m_bInThermOpticCamo)
	{
		if (m_flCamoAuxLastTime == 0)
		{
			// NEO TODO (Rain): add server style interface for accessor,
			// so we can share code
			if (m_HL2Local.m_cloakPower >= MIN_CLOAK_AUX)
			{
				m_flCamoAuxLastTime = gpGlobals->curtime;
			}
		}
		else
		{
			const float deltaTime = gpGlobals->curtime - m_flCamoAuxLastTime;
			if (deltaTime >= 1)
			{
				// NEO TODO (Rain): add interface for predicting this

				const float auxToDrain = deltaTime * CLOAK_AUX_COST;
				if (m_HL2Local.m_cloakPower <= auxToDrain)
				{
					m_HL2Local.m_cloakPower = 0.0f;
				}

				if (m_HL2Local.m_cloakPower < MIN_CLOAK_AUX)
				{
					SetCloakState(false);

					m_HL2Local.m_cloakPower = 0.0f;
					m_flCamoAuxLastTime = 0;
				}
				else
				{
					m_flCamoAuxLastTime = gpGlobals->curtime;
				}
			}
		}
	}
	else
	{
		m_flCamoAuxLastTime = 0;
	}

	if (IsAlive())
	{
		if (IsLocalPlayer() && m_bFirstAliveTick)
		{
			m_bFirstAliveTick = false;
			// NEO TODO (Adam) since the stuff in C_NEO_PLAYER::Spawn() only runs the first time a person spawns in the map, would it be worth moving some of the stuff from there here instead?
#ifdef GLOWS_ENABLE
			// Disable client side glow effects of all players
			glow_outline_effect_enable.SetValue(false);
#endif // GLOWS_ENABLE
		}
	}
	else
	{
		if (IsLocalPlayer())
		{
			m_bFirstAliveTick = true;
		}
	}

	if (IsAlive() || m_vecLean != vec3_origin)
	{
		Lean();
	}

	// Eek. See rationale for this thing in CNEO_Player::PreThink
	if (IsAirborne())
	{
		m_flLastAirborneJumpOkTime = gpGlobals->curtime;
		const float deltaTime = gpGlobals->curtime - m_flLastAirborneJumpOkTime;
		const float leeway = 0.5f;
		if (deltaTime > leeway)
		{
			m_bHasBeenAirborneForTooLongToSuperJump = false;
			m_flLastAirborneJumpOkTime = gpGlobals->curtime;
		}
		else
		{
			m_bHasBeenAirborneForTooLongToSuperJump = true;
		}
	}
	else
	{
		m_bHasBeenAirborneForTooLongToSuperJump = false;
	}

	if (m_iNeoClass == NEO_CLASS_RECON && 
		(m_afButtonPressed & IN_JUMP) && (m_nButtons & IN_SPEED) && 
		IsAllowedToSuperJump())
	{
		SuitPower_Drain(SUPER_JMP_COST);
		bool forward = m_nButtons & IN_FORWARD;
		bool backward = m_nButtons & IN_BACK;
		if (forward xor backward)
		{
			SuperJump();
		}
	}

	if (auto *indicator = GET_HUDELEMENT(CNEOHud_GameEvent))
	{
		if (m_bShowTestMessage)
		{
			indicator->SetMessage(m_pszTestMessage, sizeof(m_pszTestMessage));
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

void C_NEO_Player::Lean(void)
{
	auto vm = static_cast<CNEOPredictedViewModel*>(GetViewModel());
	if (vm)
	{
		Assert(GetBaseAnimating());
		GetBaseAnimating()->SetBoneController(0, vm->lean(this));

		//debugoverlay->AddTextOverlay(this->GetAbsOrigin() + GetViewOffset(), 0.001, "client view offset");
	}
}

void C_NEO_Player::ClientThink(void)
{
	BaseClass::ClientThink();

	if (IsCloaked())
	{ // PreThink and PostThink are only ran for local player, update every in pvs player's cloak strength here
		auto pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
		if (pLocalPlayer)
		{
			auto vel = GetAbsVelocity().Length();
			if (this == pLocalPlayer)
			{
				if (vel > 0.5) { m_flTocFactor = min(0.3f, m_flTocFactor + 0.01); } // NEO TODO (Adam) base on time rather than think rate
				else { m_flTocFactor = max(0.1f, m_flTocFactor - 0.01); }
			}
			else
			{
				if (vel > 0.5) { m_flTocFactor = 0.3f; } // 0.345f
				else { m_flTocFactor = 0.2f; } // 0.255f
			}
		}
	}	
}

static ConVar neo_this_client_speed("neo_this_client_speed", "0", FCVAR_SPONLY);
extern ConVar cl_spec_mode;

void C_NEO_Player::PostThink(void)
{
	BaseClass::PostThink();

	SetNextClientThink(CLIENT_THINK_ALWAYS);

	if (IsLocalPlayer())
	{
		neo_this_client_speed.SetValue(MIN(GetAbsVelocity().Length2D() / GetNormSpeed(), 1.0f));
	}

	if (!IsAlive())
	{
		// Undo aim zoom if just died
		if (m_bFirstDeathTick)
		{
			m_bFirstDeathTick = false;

			Weapon_SetZoom(false);
			m_bInVision = false;
			IN_LeanReset();

			if (IsLocalPlayer() && GetDeathTime() != 0 && (GetTeamNumber() == TEAM_JINRAI || GetTeamNumber() == TEAM_NSF))
			{
				SetObserverMode(OBS_MODE_DEATHCAM);
				// Fade out 8s to blackout + 2s full blackout
				ScreenFade_t sfade{
					.duration = static_cast<unsigned short>(static_cast<float>(1<<SCREENFADE_FRACBITS) * 8.0f),
					.holdTime = static_cast<unsigned short>(static_cast<float>(1<<SCREENFADE_FRACBITS) * 2.0f),
					.fadeFlags = FFADE_OUT|FFADE_PURGE,
					.r = 0,
					.g = 0,
					.b = 0,
					.a = 255,
				};
				vieweffects->Fade(sfade);

				m_Local.m_iHideHUD = HIDEHUD_HEALTH;
				cl_spec_mode.SetValue(OBS_MODE_IN_EYE);
				if (auto *viewport = gViewPortInterface->FindPanelByName("specgui"))
				{
					viewport->ShowPanel(true);
				}
				engine->ClientCmd("spec_mode");
			}
		}

		if (IsLocalPlayer() && GetObserverMode() == OBS_MODE_DEATHCAM && gpGlobals->curtime >= (GetDeathTime() + 10.0f))
		{
			// Deathcam -> None so you view your own body for a bit before proper spec
			m_iObserverMode = OBS_MODE_NONE;
			g_ClientVirtualReality.AlignTorsoAndViewToWeapon();

			// Fade in 2s from blackout
			ScreenFade_t sfade{
				.duration = static_cast<unsigned short>(static_cast<float>(1<<SCREENFADE_FRACBITS) * 2.0f),
				.holdTime = static_cast<unsigned short>(0),
				.fadeFlags = FFADE_IN|FFADE_PURGE,
				.r = 0,
				.g = 0,
				.b = 0,
				.a = 255,
			};
			vieweffects->Fade(sfade);
		}
		return;
	}
	else
	{
		if (!m_bFirstDeathTick)
		{
			m_bFirstDeathTick = true;
			if (IsLocalPlayer())
			{
				SetObserverMode(OBS_MODE_NONE);
			}
		}
	}

	CheckLeanButtons();

	if (auto *pNeoWep = static_cast<C_NEOBaseCombatWeapon *>(GetActiveWeapon()))
	{
		const bool clientAimHold = ClientWantsAimHold(this);
		if (pNeoWep->m_bInReload && !m_bPreviouslyReloading)
		{
			Weapon_SetZoom(false);
		}
		else if (CanSprint() && m_afButtonPressed & IN_SPEED)
		{
			Weapon_SetZoom(false);
		}
		else if (clientAimHold ? (m_nButtons & IN_AIM && !IsInAim()) : m_afButtonPressed & IN_AIM)
		{
			if (!CanSprint() || !(m_nButtons & IN_SPEED))
			{
				Weapon_AimToggle(pNeoWep, clientAimHold ? NEO_TOGGLE_FORCE_AIM : NEO_TOGGLE_DEFAULT);
			}
		}
		else if (clientAimHold && (m_afButtonReleased & IN_AIM))
		{
			Weapon_AimToggle(pNeoWep, NEO_TOGGLE_FORCE_UN_AIM);
		}

#if !defined( NO_ENTITY_PREDICTION )
		// Can't do aim zoom in prediction, because we can't know
		// server's reload state for our weapon with certainty.
		if (!GetPredictable() || !prediction->InPrediction())
		{
#else
		if (true) {
#endif
			m_bPreviouslyReloading = pNeoWep->m_bInReload;
		}
	}
}

void C_NEO_Player::CalcDeathCamView(Vector &eyeOrigin, QAngle &eyeAngles, float &fov)
{
	auto* pRagdoll = static_cast<C_HL2MPRagdoll*>(m_hRagdoll.Get());
	if (pRagdoll && GetClass() != NEO_CLASS_JUGGERNAUT)
	{
		// First person death cam
		pRagdoll->GetAttachment(pRagdoll->LookupAttachment("eyes"), eyeOrigin, eyeAngles);
		Vector vForward;
		AngleVectors(eyeAngles, &vForward);
		fov = GetFOV();
	}
	else if (GetClass() == NEO_CLASS_JUGGERNAUT)
	{
		Vector vTarget = vec3_origin;
		if (m_hDroppedJuggernautItem)
		{
			vTarget = m_hDroppedJuggernautItem->WorldSpaceCenter();
		}
		else
		{
			vTarget = NEORules()->GetJuggernautMarkerPos();

		}

		eyeOrigin = vTarget + Vector(80, 80, 80);
		trace_t tr;
		CTraceFilterWorldOnly traceFilter;
		UTIL_TraceLine(vTarget, eyeOrigin, MASK_OPAQUE, &traceFilter, &tr);
		eyeOrigin = vTarget + ((Vector(75, 75, 75) * tr.fraction));

		Vector vDir = vTarget - eyeOrigin;
		VectorNormalize(vDir);
		VectorAngles(vDir, eyeAngles);
		fov = GetFOV();
	}
	else
	{
		// Fallback just in-case it somehow doesn't do m_hRagdoll
		BaseClass::CalcDeathCamView(eyeOrigin, eyeAngles, fov);
	}
}

void C_NEO_Player::TeamChange(int iNewTeam)
{
	BaseClass::TeamChange(iNewTeam);
}

bool C_NEO_Player::IsAllowedToSuperJump(void)
{
	if (!IsSprinting())
		return false;

	if (IsCarryingGhost())
		return false;

	if (GetMoveParent())
		return false;

	if (IsPlayerUnderwater())
		return false;

	// Can't superjump whilst airborne (although it is kind of cool)
	if (m_bHasBeenAirborneForTooLongToSuperJump)
		return false;

	// Only superjump if we have a reasonable jump direction in mind
	// NEO TODO (Rain): should we support sideways superjumping?
	if ((m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)) == 0)
	{
		return false;
	}

	// The suit check is for prediction only, actual power drain happens serverside
	if (m_HL2Local.m_flSuitPower < SUPER_JMP_COST)
		return false;

	if (SUPER_JMP_DELAY_BETWEEN_JUMPS > 0)
	{
		m_flLastSuperJumpTime = gpGlobals->curtime;
		const float deltaTime = gpGlobals->curtime - m_flLastSuperJumpTime;
		if (deltaTime > SUPER_JMP_DELAY_BETWEEN_JUMPS)
			return false;

		m_flLastSuperJumpTime = gpGlobals->curtime;
	}

	return true;
}

// This is applied for prediction purposes. It should match CNEO_Player's method.
void C_NEO_Player::SuperJump(void)
{
	Vector forward;
	AngleVectors(EyeAngles(), &forward);

	// We don't give an upwards boost aside from regular jump
	forward.z = 0;

	// Flip direction if jumping backwards
	if (m_nButtons & IN_BACK)
	{
		forward = -forward;
	}

	float boostIntensity = GetPlayerMaxSpeed();
	if (m_nButtons & (IN_MOVELEFT | IN_MOVERIGHT))
	{
		constexpr float sideWaysNerf = 0.70710678118; // 1 / sqrt(2);
		boostIntensity *= sideWaysNerf;
	}

	ApplyAbsVelocityImpulse(forward * boostIntensity);
}

float C_NEO_Player::CloakPower_CurrentVisualPercentage(void) const
{
	const float cloakPowerRounded = roundf(m_HL2Local.m_cloakPower);
	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		return (cloakPowerRounded / 13.0f) * 100.0f;
	case NEO_CLASS_ASSAULT:
		return (cloakPowerRounded / 8.0f) * 100.0f;
	default:
		break;
	}
	return 0.0f;
}

void C_NEO_Player::Spawn( void )
{
	BaseClass::Spawn();

	m_bLastTickInThermOpticCamo = m_bInThermOpticCamo = false;
	m_flCamoAuxLastTime = 0;

	m_bInVision = false;
	m_nVisionLastTick = 0;
	m_bInLean = NEO_LEAN_NONE;

	static_assert(_ARRAYSIZE(m_rfAttackersScores) == MAX_PLAYERS);
	static_assert(_ARRAYSIZE(m_rfAttackersAccumlator) == MAX_PLAYERS);
	static_assert(_ARRAYSIZE(m_rfAttackersHits) == MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_rfAttackersScores.GetForModify(i) = 0;
		m_rfAttackersAccumlator.GetForModify(i) = 0.0f;
		m_rfAttackersHits.GetForModify(i) = 0;
	}
	V_memset(m_rfNeoPlayerIdxsKilledByLocal, 0, sizeof(m_rfNeoPlayerIdxsKilledByLocal));

	Weapon_SetZoom(false);

	SetViewOffset(VEC_VIEW_NEOSCALE(this));

	auto *localPlayer = C_NEO_Player::GetLocalNEOPlayer();

	if (localPlayer == nullptr || localPlayer == this)
	{
		// NEO NOTE (nullsystem): Reset Vis/Enabled/MouseInput/Cursor state here, otherwise it can get stuck at situations
		for (const auto pname : {PANEL_CLASS, PANEL_TEAM, PANEL_NEO_LOADOUT})
		{
			if (auto *panel = static_cast<vgui::EditablePanel*>
					(GetClientModeNormal()->GetViewport()->FindChildByName(pname)))
			{
				panel->SetVisible(false);
				panel->SetEnabled(false);
				panel->SetMouseInputEnabled(false);
				panel->SetCursorAlwaysVisible(false);
				//panel->SetKeyBoardInputEnabled(false);
			}
		}

		for (auto *hud : gHUD.m_HudList)
		{
			if (auto *neoHud = dynamic_cast<CNEOHud_ChildElement *>(hud))
			{
				neoHud->resetLastUpdateTime();
				neoHud->resetHUDState();
			}
		}
	}
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

void C_NEO_Player::Weapon_Drop(C_NEOBaseCombatWeapon *pWeapon)
{
	m_bIneligibleForLoadoutPick = true;
	Weapon_SetZoom(false);

	if (pWeapon->IsGhost())
	{
		pWeapon->Holster(NULL);
		assert_cast<C_WeaponGhost*>(pWeapon)->HandleGhostUnequip();
	}
	else if (pWeapon->GetNeoWepBits() & NEO_WEP_SUPA7)
	{
		assert_cast<C_WeaponSupa7*>(pWeapon)->ClearDelayedInputs();
	}
}

void C_NEO_Player::StartSprinting(void)
{
	if (IsCarryingGhost())
	{
		return;
	}

	BaseClass::StartSprinting();
}

void C_NEO_Player::StopSprinting(void)
{
	m_fIsSprinting = false;
	IN_SpeedReset();
}

bool C_NEO_Player::CanSprint(void)
{
	if (IsCarryingGhost() || m_iNeoClass == NEO_CLASS_SUPPORT)
	{
		return false;
	}

	return true;
}

void C_NEO_Player::StartWalking(void)
{
	m_fIsWalking = true;
}

void C_NEO_Player::StopWalking(void)
{
	m_fIsWalking = false;
}

float C_NEO_Player::GetCrouchSpeed_WithActiveWepEncumberment(void) const
{
	return GetCrouchSpeed() * GetActiveWeaponSpeedScale();
}

float C_NEO_Player::GetNormSpeed_WithActiveWepEncumberment(void) const
{
	return GetNormSpeed() * GetActiveWeaponSpeedScale();
}

float C_NEO_Player::GetSprintSpeed_WithActiveWepEncumberment(void) const
{
	return GetSprintSpeed() * GetActiveWeaponSpeedScale();
}

float C_NEO_Player::GetCrouchSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetCrouchSpeed() * pNeoWep->GetSpeedScale();
}

float C_NEO_Player::GetNormSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetNormSpeed() * pNeoWep->GetSpeedScale();
}

float C_NEO_Player::GetSprintSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetSprintSpeed() * pNeoWep->GetSpeedScale();
}

float C_NEO_Player::GetCrouchSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_CROUCH_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_CROUCH_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_CROUCH_SPEED;
	case NEO_CLASS_VIP:
		return NEO_VIP_CROUCH_SPEED;
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_CROUCH_SPEED;
	default:
		return (NEO_BASE_SPEED * NEO_CROUCH_MODIFIER);
	}
}

float C_NEO_Player::GetNormSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_BASE_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_BASE_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_BASE_SPEED;
	case NEO_CLASS_VIP:
		return NEO_VIP_BASE_SPEED;
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_BASE_SPEED;
	default:
		return NEO_BASE_SPEED;
	}
}

float C_NEO_Player::GetSprintSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_SPRINT_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_SPRINT_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_SPRINT_SPEED;
	case NEO_CLASS_VIP:
		return NEO_VIP_SPRINT_SPEED;
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_SPRINT_SPEED;
	default:
		return NEO_BASE_SPEED; // No generic sprint modifier; default speed.
	}
}

float C_NEO_Player::GetActiveWeaponSpeedScale() const
{
	auto pWep = static_cast<C_NEOBaseCombatWeapon *>(GetActiveWeapon());
	return (pWep ? pWep->GetSpeedScale() : 1.0f);
}

void C_NEO_Player::Weapon_AimToggle(C_NEOBaseCombatWeapon *pNeoWep, const NeoWeponAimToggleE toggleType)
{
	// This implies the wep ptr is valid, so we don't bother checking
	if (IsAllowedToZoom(pNeoWep))
	{
		if (toggleType != NEO_TOGGLE_FORCE_UN_AIM)
		{
			const bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) == HIDEHUD_CROSSHAIR;
			Weapon_SetZoom(showCrosshair);
		}
		else if (toggleType != NEO_TOGGLE_FORCE_AIM)
		{
			Weapon_SetZoom(false);
		}
	}
}

void C_NEO_Player::Weapon_SetZoom(const bool bZoomIn)
{
	if (bZoomIn == m_bInAim)
	{
		return;
	}

	float zoomSpeedSecs = NEO_ZOOM_SPEED;

#if(0)
#if !defined( NO_ENTITY_PREDICTION )
	if (!prediction->InPrediction() && IsLocalPlayer())
	{
		if (GetPredictable())
		{
			zoomSpeedSecs *= gpGlobals->interpolation_amount;
		}
	}
#endif
#endif

	if (bZoomIn)
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;
	}

	const int fov = GetDefaultFOV();
	SetFOV(static_cast<CBaseEntity *>(this), bZoomIn ? NeoAimFOV(fov, GetActiveWeapon()) : fov, zoomSpeedSecs);
	m_bInAim = bZoomIn;
}

bool C_NEO_Player::IsCarryingGhost(void) const
{
	return GetNeoWepWithBits(this, NEO_WEP_GHOST) != NULL;
}

const Vector C_NEO_Player::GetPlayerMins(void) const
{
	return VEC_DUCK_HULL_MIN_SCALED(this);
}

const Vector C_NEO_Player::GetPlayerMaxs(void) const
{
	return VEC_DUCK_HULL_MAX_SCALED(this);
}

void C_NEO_Player::PlayCloakSound(void)
{
	C_RecipientFilter filter;
	filter.AddRecipient(this);
	filter.MakeReliable();

	static int tocOn = CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOn");
	static int tocOff = CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOff");

	EmitSound_t params;
	params.m_bEmitCloseCaption = false;
	params.m_hSoundScriptHandle = (m_bInThermOpticCamo ? tocOn : tocOff);
	params.m_pOrigin = &GetAbsOrigin();
	params.m_nChannel = CHAN_VOICE;

	EmitSound(filter, entindex(), params);
}

void C_NEO_Player::SetCloakState(bool state)
{
	m_bInThermOpticCamo = state;

	void (CBaseCombatWeapon:: * setShadowState)(int) = m_bInThermOpticCamo ? &CBaseCombatWeapon::AddEffects 
																		   : &CBaseCombatWeapon::RemoveEffects;

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (CBaseCombatWeapon* weapon = GetWeapon(i))
		{
			(weapon->*setShadowState)(EF_NOSHADOW);
		}
	}
}

void C_NEO_Player::PreDataUpdate(DataUpdateType_t updateType)
{
	if (updateType == DATA_UPDATE_DATATABLE_CHANGED)
	{
		if (gpGlobals->tickcount - m_nVisionLastTick < TIME_TO_TICKS(0.1f))
		{
			return;
		}
		else
		{
			m_bIsAllowedToToggleVision = true;
		}
	}

	BaseClass::PreDataUpdate(updateType);
}

extern ConVar sv_neo_wep_dmg_modifier;

// NEO NOTE (Rain): doesn't seem to be implemented at all clientside?
// Don't need to do this, unless we want it for prediction with proper implementation later.
// Keeping it for now.
void C_NEO_Player::ModifyFireBulletsDamage(CTakeDamageInfo* dmgInfo)
{
	dmgInfo->SetDamage(dmgInfo->GetDamage() * sv_neo_wep_dmg_modifier.GetFloat());
}

const char *C_NEO_Player::GetOverrideStepSound(const char *pBaseStepSound)
{
	if (GetClass() == NEO_CLASS_JUGGERNAUT)
	{
		if (!IsSprinting())
		{
			if (m_Local.m_nStepside)
			{
				return "JGR56.FootstepLeft";
			}
			else
			{
				return "JGR56.FootstepRight";
			}
		}
		else
		{
			if (m_Local.m_nStepside)
			{
				return "JGR56.RunFootstepLeft";
			}
			else
			{
				return "JGR56.RunFootstepRight";
			}
		}
	}

	return BaseClass::GetOverrideStepSound(pBaseStepSound);
}

// Receive server message to smooth out player takeover
void __MsgFunc_CSpectatorTakeoverPlayer(bf_read &msg)
{
	int spectatorTakingOverUserID = msg.ReadLong();
	int playerTakeoverTargetUserID = msg.ReadLong();

	C_NEO_Player* pSpectatorTakingOver = USERID2NEOPLAYER(spectatorTakingOverUserID);
	C_NEO_Player* pPlayerTakeoverTarget = USERID2NEOPLAYER(playerTakeoverTargetUserID);


	if (!pPlayerTakeoverTarget || !pSpectatorTakingOver)
	{
		return;
	}

	// Save for later in C_NEO_Player::OnDataChanged
	pSpectatorTakingOver->m_hSpectatorTakeoverTarget = pPlayerTakeoverTarget;
}

CUserMessageRegister CSpectatorTakeoverPlayerRegistration("CSpectatorTakeoverPlayer", __MsgFunc_CSpectatorTakeoverPlayer);

void C_NEO_Player::CSpectatorTakeoverPlayerUpdateOnDataChanged()
{
    if ( C_NEO_Player* pTarget = m_hSpectatorTakeoverTarget.Get() )
	{
        // Check if our model is stable from server & matches the target
        if (GetModel() && (GetModel() == pTarget->GetModel()))
        {
            CSpectatorTakeoverPlayerUpdate(pTarget);
            m_hSpectatorTakeoverTarget = nullptr;
        }
    }
}

void C_NEO_Player::CSpectatorTakeoverPlayerUpdate(C_NEO_Player* pPlayerTakeoverTarget)
{
	if (!pPlayerTakeoverTarget)
	{
		return;
	}

	m_nSkin = pPlayerTakeoverTarget->m_iNeoSkin;
	m_iNeoClass = pPlayerTakeoverTarget->m_iNeoClass;
	m_iLoadoutWepChoice = pPlayerTakeoverTarget->m_iLoadoutWepChoice;
	m_iNextSpawnClassChoice = pPlayerTakeoverTarget->m_iNextSpawnClassChoice;

	m_bInThermOpticCamo = pPlayerTakeoverTarget->m_bInThermOpticCamo;
	m_bInVision = pPlayerTakeoverTarget->m_bInVision;
	m_bInAim = pPlayerTakeoverTarget->m_bInAim;
	m_bCarryingGhost = pPlayerTakeoverTarget->m_bCarryingGhost;
	m_bIneligibleForLoadoutPick = pPlayerTakeoverTarget->m_bIneligibleForLoadoutPick;
	m_bInLean = pPlayerTakeoverTarget->m_bInLean;

	m_flCamoAuxLastTime = pPlayerTakeoverTarget->m_flCamoAuxLastTime;
	m_nVisionLastTick = pPlayerTakeoverTarget->m_nVisionLastTick;
	m_flLastAirborneJumpOkTime = pPlayerTakeoverTarget->m_flLastAirborneJumpOkTime;
	m_flLastSuperJumpTime = pPlayerTakeoverTarget->m_flLastSuperJumpTime;
	m_bPreviouslyReloading = pPlayerTakeoverTarget->m_bPreviouslyReloading;
	m_bLastTickInThermOpticCamo = pPlayerTakeoverTarget->m_bLastTickInThermOpticCamo;
	m_bIsAllowedToToggleVision = pPlayerTakeoverTarget->m_bIsAllowedToToggleVision;
	m_flTocFactor = pPlayerTakeoverTarget->m_flTocFactor;

	pPlayerTakeoverTarget->SnatchModelInstance(this);
	SetAbsOrigin(pPlayerTakeoverTarget->GetAbsOrigin());
	SetAbsAngles(pPlayerTakeoverTarget->GetAbsAngles());
	SetAbsVelocity(pPlayerTakeoverTarget->GetAbsVelocity());
	SetLocalAngles(pPlayerTakeoverTarget->GetLocalAngles());
	pl.v_angle = pPlayerTakeoverTarget->pl.v_angle; // mimic SnapEyeAngles in neo_player
}

