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

#include "baseviewmodel_shared.h"

#include "prediction.h"

#include "weapon_ghost.h"

#include <engine/ivdebugoverlay.h>

// Don't alias here
#if defined( CNEO_Player )
#undef CNEO_Player	
#endif

LINK_ENTITY_TO_CLASS(player, C_NEO_Player);

IMPLEMENT_CLIENTCLASS_DT(C_NEO_Player, DT_NEO_Player, CNEO_Player)
	RecvPropInt(RECVINFO(m_nNeoSkin)),
	RecvPropInt(RECVINFO(m_nCyborgClass)),
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
}

C_NEO_Player::~C_NEO_Player()
{
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

	DrawCompass();
}

void C_NEO_Player::ClientThink(void)
{
	BaseClass::ClientThink();
}

void C_NEO_Player::PostThink(void)
{
	BaseClass::PostThink();

	//DevMsg("Roll: %f\n", m_angEyeAngles[2]);
}

void C_NEO_Player::Spawn( void )
{
	BaseClass::Spawn();
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
		ghost->HandleGhostUnequipSound();
	}
}

ConVar neo_cl_hud_compass_enabled("neo_cl_hud_compass_enabled", "1", FCVAR_USERINFO,
	"Whether the HUD compass is enabled or not.", true, 0.0f, true, 1.0f);
ConVar neo_cl_hud_compass_pos_x("neo_cl_hud_compass_pos_x", "0.45", FCVAR_USERINFO,
	"Horizontal position of the compass, in range 0 to 1.", true, 0.0f, true, 1.0f);
ConVar neo_cl_hud_compass_pos_y("neo_cl_hud_compass_pos_y", "0.925", FCVAR_USERINFO,
	"Vertical position of the compass, in range 0 to 1.", true, 0.0f, true, 1.0f);
ConVar neo_cl_hud_compass_color_r("neo_cl_hud_compass_color_r", "190", FCVAR_USERINFO,
	"Red color value of the compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_compass_color_g("neo_cl_hud_compass_color_g", "185", FCVAR_USERINFO,
	"Green color value of the compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_compass_color_b("neo_cl_hud_compass_color_b", "205", FCVAR_USERINFO,
	"Blue value of the compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_compass_color_a("neo_cl_hud_compass_color_a", "255", FCVAR_USERINFO,
	"Alpha color value of the compass, in range 0 - 255.", true, 0.0f, true, 255.0f);

// Purpose: Draw a simple compass to the bottom of player's screen.
// NEO TODO (Rain): figure out the HUD design and add fancier textured compass.
inline void C_NEO_Player::DrawCompass(void)
{
	if (!neo_cl_hud_compass_enabled.GetBool())
	{
		return;
	}

	// Direction in -180 to 180
	float angle = EyeAngles()[YAW];

	// Bring us back to safety
	if (angle > 180)
	{
		angle -= 360;
	}
	else if (angle < -180)
	{
		angle += 360;
	}

	// Char representation of the compass strip
	const char rose[] = "N -- ne -- E -- se -- S -- sw -- W -- nw -- ";

	// One compass tick represents this many degrees of rotation
	const int unitAccuracy = RoundFloatToInt(360.0f / sizeof(rose));

	// How many characters should be visible around each side of the needle position
	const int numCharsVisibleAroundNeedle = 6;

	// Both sides + center + terminator
	char compass[numCharsVisibleAroundNeedle * 2 + 2];
	int i;
	for (i = 0; i < sizeof(compass) - 1; i++)
	{
		int offset = (angle / unitAccuracy) - numCharsVisibleAroundNeedle;
		if (offset < 0)
		{
			offset += sizeof(rose);
		}

		// Get our index by circling around the compass strip.
		// We do modulo -1, because sizeof would land us on NULL
		// and terminate the string early.
		const int wrappedIndex = (offset + i) % (sizeof(rose) - 1);

		compass[i] = rose[wrappedIndex];
	}
	// Finally, make sure we have a null terminator
	compass[i] = '\0';

	// Draw the compass for this frame
	debugoverlay->AddScreenTextOverlay(
		neo_cl_hud_compass_pos_x.GetFloat(),
		neo_cl_hud_compass_pos_y.GetFloat(),
		gpGlobals->frametime,
		neo_cl_hud_compass_color_r.GetInt(),
		neo_cl_hud_compass_color_g.GetInt(),
		neo_cl_hud_compass_color_b.GetInt(),
		neo_cl_hud_compass_color_a.GetInt(),
		compass);
}