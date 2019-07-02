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
#include "ui/neo_hud_compass.h"
#include "ui/neo_hud_game_event.h"
#include "ui/neo_hud_ghost_marker.h"

#include "baseviewmodel_shared.h"

#include "prediction.h"

#include "weapon_ghost.h"

#include <engine/ivdebugoverlay.h>

#include "engine/ienginesound.h"

// Don't alias here
#if defined( CNEO_Player )
#undef CNEO_Player	
#endif

LINK_ENTITY_TO_CLASS(player, C_NEO_Player);

IMPLEMENT_CLIENTCLASS_DT(C_NEO_Player, DT_NEO_Player, CNEO_Player)
	RecvPropInt(RECVINFO(m_nNeoSkin)),
	RecvPropInt(RECVINFO(m_nCyborgClass)),

	RecvPropBool(RECVINFO(m_bShowTestMessage)),
	RecvPropString(RECVINFO(m_pszTestMessage)),

	RecvPropInt(RECVINFO(m_iCapTeam)),

	RecvPropVector(RECVINFO(m_vecGhostMarkerPos)),
	RecvPropInt(RECVINFO(m_iGhosterTeam)),
	RecvPropBool(RECVINFO(m_bGhostExists)),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_NEO_Player)
END_PREDICTION_DATA()

ConVar cl_autoreload_when_empty("cl_autoreload_when_empty", "1", FCVAR_USERINFO,
	"Automatically start reloading when the active weapon becomes empty.",
	true, 0.0f, true, 1.0f);

ConVar cl_drawhud_quickinfo("cl_drawhud_quickinfo", "0", 0,
	"Whether to display HL2 style ammo/health info near crosshair.",
	true, 0.0f, true, 1.0f);

C_NEO_Player::C_NEO_Player()
{
	m_pCompass = new CNEOHud_Compass("compass");
	m_pCompass->SetOwner(this);

	m_pHudEvent_Test = new CNEOHud_GameEvent("hudEvent_Test");
	m_pHudEvent_Test->SetMessage("Test message");

	m_iCapTeam = TEAM_UNASSIGNED;
	m_iGhosterTeam = TEAM_UNASSIGNED;

	m_pGhostMarker = new CNEOHud_GhostMarker("ghostMarker");

	m_vecGhostMarkerPos = vec3_origin;
	m_bGhostExists = false;
}

C_NEO_Player::~C_NEO_Player()
{
	if (m_pCompass)
	{
		m_pCompass->MarkForDeletion();
		delete m_pCompass;
	}

	if (m_pHudEvent_Test)
	{
		m_pHudEvent_Test->MarkForDeletion();
		delete m_pHudEvent_Test;
	}

	if (m_pGhostMarker)
	{
		m_pGhostMarker->MarkForDeletion();
		delete m_pGhostMarker;
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
	return BaseClass::DrawModel(flags);
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
}

void C_NEO_Player::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	static bool onceOnly = true;

	// NEO HACK/FIXME (Rain): this should be issued from serverside gamerules with player filter,
	// using proper precached soundscript entries
	if (m_iCapTeam != TEAM_UNASSIGNED)
	{
		if (onceOnly)
		{
			if (m_iCapTeam == TEAM_JINRAI)
			{
				/*
				EmitSound("Victory.Jinrai");

				EmitSound("sound/gameplay/jinrai.mp3");
				*/

				enginesound->EmitAmbientSound("gameplay/jinrai.mp3", 1.f);
			}
			else if (m_iCapTeam == TEAM_NSF)
			{
				/*
				EmitSound("Victory.NSF");

				EmitSound("sound/gameplay/nsf.mp3");
				*/

				enginesound->EmitAmbientSound("gameplay/nsf.mp3", 1.f);
			}
			else
			{
				EmitSound("Victory.Draw");
			}

			onceOnly = !onceOnly;
		}
	}
	else
	{
		if (onceOnly != true)
		{
			onceOnly = true;
		}
	}
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

	if (m_bShowTestMessage)
	{
		m_pHudEvent_Test->SetMessage(m_pszTestMessage);
		m_pHudEvent_Test->SetVisible(true);
	}

	if (!m_bGhostExists)
	{
		m_pGhostMarker->SetVisible(false);
	}
	else
	{
		const float distance = METERS_PER_INCH *
			Vector(GetAbsOrigin() - m_vecGhostMarkerPos).Length2D();

		// NEO HACK (Rain): We should test if we're holding a ghost
		// instead of relying on a distance check.
		if (m_iGhosterTeam != GetTeamNumber() || distance > 0.2)
		{
			m_pGhostMarker->SetVisible(true);

			int ghostMarkerX, ghostMarkerY;
			GetVectorInScreenSpace(m_vecGhostMarkerPos, ghostMarkerX, ghostMarkerY);

			m_pGhostMarker->SetScreenPosition(ghostMarkerX, ghostMarkerY);
			m_pGhostMarker->SetGhostingTeam(m_iGhosterTeam);


			m_pGhostMarker->SetGhostDistance(distance);
		}
		else
		{
			m_pGhostMarker->SetVisible(false);
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
		m_pHudEvent_Test->SetVisible(false);
		previouslyPreparing = false;
	}
}

void C_NEO_Player::Spawn( void )
{
	BaseClass::Spawn();

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
