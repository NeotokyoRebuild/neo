#include "cbase.h"
#include "neo_predicted_viewmodel.h"

#include "in_buttons.h"
#include "neo_gamerules.h"
#include "weapon_hl2mpbase.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"

#include "engine/ivdebugoverlay.h"
#include "iinput.h"
#include "inetchannelinfo.h"
#include "model_types.h"
#include "prediction.h"
#include "viewrender.h"
#include "r_efx.h"
#include "dlight.h"
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED(NEOPredictedViewModel, DT_NEOPredictedViewModel)

BEGIN_NETWORK_TABLE(CNEOPredictedViewModel, DT_NEOPredictedViewModel)
#ifdef CLIENT_DLL
RecvPropFloat(RECVINFO(m_flYPrevious)),
#else
SendPropFloat(SENDINFO(m_flYPrevious)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOPredictedViewModel)
DEFINE_PRED_FIELD(m_flYPrevious, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel, CNEOPredictedViewModel);

CNEOPredictedViewModel::CNEOPredictedViewModel()
#ifdef CLIENT_DLL
	: m_iv_flYPrevious("CNEOPredictedViewModel::m_iv_flYPrevious")
#endif
{
#ifdef CLIENT_DLL
#ifdef DEBUG
	IMaterial *pass = materials->FindMaterial("dev/toc_cloakpass", TEXTURE_GROUP_CLIENT_EFFECTS);
	Assert(pass && pass->IsPrecached());
#endif

	AddVar(&m_flYPrevious, &m_iv_flYPrevious, LATCH_SIMULATION_VAR);
#endif

	m_flYPrevious = 0;
	m_flStartAimingChange = 0;
	m_bViewAim = false;
}

CNEOPredictedViewModel::~CNEOPredictedViewModel()
{
}

#ifdef CLIENT_DLL
void CNEOPredictedViewModel::DrawRenderToTextureDebugInfo(IClientRenderable* pRenderable,
	const Vector& mins, const Vector& maxs, const Vector &rgbColor,
	const char *message, const Vector &vecOrigin)
{
	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecSize;
	VectorSubtract( maxs, mins, vecSize );

	//Vector vecOrigin = pRenderable->GetRenderOrigin() + renderOffset;
	Vector start, end, end2;

	VectorMA( vecOrigin, mins.x, vec[0], start );
	VectorMA( start, mins.y, vec[1], start );
	VectorMA( start, mins.z, vec[2], start );

	VectorMA( start, vecSize.x, vec[0], end );
	VectorMA( end, vecSize.z, vec[2], end2 );

	const float duration = 0.01f;
	const bool noDepthTest = true;

	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);
	debugoverlay->AddLineOverlay( end2, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( start, vecSize.y, vec[1], end );
	VectorMA( end, vecSize.z, vec[2], end2 );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);
	debugoverlay->AddLineOverlay( end2, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( start, vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);
	
	start = end;
	VectorMA( start, vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( start, vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( end, vecSize.x, vec[0], start );
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( start, -vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	start = end;
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], noDepthTest, duration);

	C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
	int lineOffset = V_stristr("end", message) ? 1 : 0;
	if ( pEnt )
	{
		debugoverlay->AddTextOverlay( vecOrigin, lineOffset, "%s -- ent %d", message, pEnt->entindex() );
	}
	else
	{
		debugoverlay->AddTextOverlay( vecOrigin, lineOffset, "%s -- renderable %X", message, (size_t)pRenderable );
	}
}
#endif

ConVar neo_lean_debug_draw_hull("neo_lean_debug_draw_hull", "0", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_speed("neo_lean_speed", "0.333", FCVAR_REPLICATED | FCVAR_CHEAT, "Lean speed scale", true, 0.0, false, 1000.0);
// Original Neotokyo with the latest leftlean fix uses 7 for leftlean and 15 for rightlean yaw slide.
ConVar neo_lean_yaw_peek_left_amount("neo_lean_yaw_peek_left_amount", "7.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full left lean view reach.", true, 0.001f, false, 0);
ConVar neo_lean_yaw_peek_right_amount("neo_lean_yaw_peek_right_amount", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full right lean view reach.", true, 0.001f, false, 0);
ConVar neo_lean_angle_percentage("neo_lean_angle_percentage", "0.75", FCVAR_REPLICATED | FCVAR_CHEAT, "for adjusting the actual angle of lean to a percentage of lean.", true, 0.0, true, 1.0);
ConVar neo_lean_tp_exaggerate_scale("neo_lean_tp_exaggerate_scale", "2", FCVAR_REPLICATED | FCVAR_CHEAT, "How much more to scale leaning motion in 3rd person animation vs 1st person viewmodel.", true, 0.0, false, 0);
ConVar neo_lean_fp_lower_eyes_scale("neo_lean_fp_lower_eyes_scale", "2.333", FCVAR_REPLICATED | FCVAR_CHEAT, "Multiplier for how low to bring eye-level (and hence bullet line level) whilst leaning.", true, 0.0, false, 0);

#if(0)
ConVar neo_lean_thirdperson_roll_lerp_scale("neo_lean_thirdperson_roll_lerp_scale", "5",
	FCVAR_REPLICATED | FCVAR_CHEAT, "Multiplier for 3rd person lean roll lerping.", true, 0.0, false, 0);
#endif

float CNEOPredictedViewModel::freeRoomForLean(float leanAmount, CNEO_Player *player){
	const Vector playerDefaultViewPos = player->GetAbsOrigin();
	Vector deltaPlayerViewPos(0, leanAmount, 0);
	VectorYawRotate(deltaPlayerViewPos, player->LocalEyeAngles().y, deltaPlayerViewPos);
	const Vector leanEndPos = playerDefaultViewPos + deltaPlayerViewPos;

	// We can only lean through stuff that isn't solid for us
	CTraceFilterNoNPCsOrPlayer filter(player, COLLISION_GROUP_PLAYER_MOVEMENT);

	// Player hull size, dependent on current ducking status
	Vector hullMins, hullMaxs;
	// Need this much z clearance to not "bump" our head whilst leaning
	const Vector groundClearance(0, 0, 30);

#if(0) // same view limits regardless of player class
#define STAND_MINS (NEORules()->GetViewVectors()->m_vHullMin + groundClearance)
#define STAND_MAXS (NEORules()->GetViewVectors()->m_vHullMax)
#define DUCK_MINS (NEORules()->GetViewVectors()->m_vDuckHullMin + groundClearance)
#define DUCK_MAXS (NEORules()->GetViewVectors()->m_vDuckHullMax)
#else // class hull specific limits
#define STAND_MINS (VEC_HULL_MIN_SCALED(player) + groundClearance)
#define STAND_MAXS (VEC_HULL_MAX_SCALED(player))
#define DUCK_MINS (VEC_DUCK_HULL_MIN_SCALED(player) + groundClearance)
#define DUCK_MAXS (VEC_DUCK_HULL_MAX_SCALED(player))
#endif

	if (player->GetFlags() & FL_DUCKING)
	{
		hullMins = DUCK_MINS;
		hullMaxs = DUCK_MAXS;
	}
	else
	{
		hullMins = STAND_MINS;
		hullMaxs = STAND_MAXS;
	}

	Assert(hullMins.IsValid() && hullMaxs.IsValid());

	trace_t trace;
	UTIL_TraceHull(playerDefaultViewPos, leanEndPos, hullMins, hullMaxs, player->PhysicsSolidMaskForEntity(), &filter, &trace);

#ifdef CLIENT_DLL
	if (neo_lean_debug_draw_hull.GetBool())
	{
		// Z offset to avoid text overlap; DrawRenderToTextureDebugInfo will also want to print text around similar area.
		const Vector debugOffset = Vector(playerDefaultViewPos) + Vector(0, 0, -4);
		const Vector color = Vector(20, 255, 0);
		debugoverlay->AddTextOverlay(debugOffset, 0.0001, "x: %f\n y: %f\n z:%f\n", playerDefaultViewPos.x, playerDefaultViewPos.y, playerDefaultViewPos.z);
		player->GetNEOViewModel()->DrawRenderToTextureDebugInfo(player, hullMins, hullMaxs, color, "freeRoomForLean", player->GetAbsOrigin());
	}
#endif

	return roundf(trace.startpos.DistTo(trace.endpos) * 100) / 100;
}

#ifdef CLIENT_DLL
void CNEOPredictedViewModel::PostDataUpdate(DataUpdateType_t updateType)
{
	SetNextClientThink(CLIENT_THINK_ALWAYS);
	BaseClass::PostDataUpdate(updateType);
}

void CNEOPredictedViewModel::ClientThink()
{
	SetNextClientThink(CLIENT_THINK_ALWAYS);
	BaseClass::ClientThink();
}

extern ConVar mat_neo_toc_test;
int CNEOPredictedViewModel::DrawModel(int flags)
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());

	if (pPlayer)
	{
		if (pPlayer->IsCloaked())
		{
			IMaterial *pass = materials->FindMaterial("models/player/toc", TEXTURE_GROUP_VIEW_MODEL);
			Assert(pass && !pass->IsErrorMaterial());

			if (pass && !pass->IsErrorMaterial())
			{
				if (!pass->IsPrecached())
				{
					PrecacheMaterial(pass->GetName());
					Assert(pass->IsPrecached());
				}

				mat_neo_toc_test.SetValue(pPlayer->GetCloakFactor());

				modelrender->ForcedMaterialOverride(pass);
				int ret = BaseClass::DrawModel(flags);
				modelrender->ForcedMaterialOverride(NULL);
				return ret;
			}
			
			return 0;
		}
		if (pPlayer->GetClass() == NEO_CLASS_SUPPORT && pPlayer->IsInVision())
		{
			IMaterial* pass = materials->FindMaterial("dev/thermal_third", TEXTURE_GROUP_MODEL);
			Assert(pass && !pass->IsErrorMaterial());

			if (pass && !pass->IsErrorMaterial())
			{
				// Render
				modelrender->ForcedMaterialOverride(pass);
				int ret = BaseClass::DrawModel(flags);
				modelrender->ForcedMaterialOverride(NULL);

				return ret;
			}
		}
	}

	return BaseClass::DrawModel(flags);
}
#endif

static inline float calculateLeanAngle(float freeRoom, CNEO_Player *player){
#define HIP_TO_HEAD_HEIGHT 41.0f
	return -RAD2DEG(atan2(freeRoom, HIP_TO_HEAD_HEIGHT)) * neo_lean_angle_percentage.GetFloat();
}

float GetLeanRatio(const float leanAngle)
{
	return fabs(leanAngle) / ((leanAngle < 0) ? neo_lean_yaw_peek_left_amount.GetFloat() : neo_lean_yaw_peek_right_amount.GetFloat());
}

float CNEOPredictedViewModel::lean(CNEO_Player *player){
	Assert(player);
#ifdef CLIENT_DLL
	input->ExtraMouseSample(gpGlobals->frametime, 1);
#endif
	QAngle viewAng = player->LocalEyeAngles();
	float Ycurrent = m_flYPrevious;
	float Yfinal = 0;

	if (player->IsAlive())
	{
		switch (player->m_bInLean.Get())
		{
		case NEO_LEAN_LEFT:
			Yfinal = freeRoomForLean(neo_lean_yaw_peek_left_amount.GetFloat(), player);
			break;
		case NEO_LEAN_RIGHT:
			Yfinal = -freeRoomForLean(-neo_lean_yaw_peek_right_amount.GetFloat(), player);
			break;
		default:
			// not leaning, or leaning both ways; move towards zero
			break;
		}
	}

	if (!(player->GetFlags() & FL_ONGROUND)) {
		//mid-air; move towards zero
		Yfinal = 0;
	}

	const float dY = Yfinal - Ycurrent;


	if (dY != 0){
		const float leanStep = 0.25f;

		// Almost done leaning; snap to zero to avoid back-and-forth "wiggle"
		if (fabs(dY) - leanStep < 0) {
			Ycurrent = Yfinal;
		}
		else {
			Ycurrent = Lerp(leanStep * neo_lean_speed.GetFloat(), Ycurrent, Yfinal);
		}
	}

	Vector viewOffset(0, 0, 0);
	viewOffset.y = m_flYPrevious = Ycurrent;

	VectorYawRotate(viewOffset, viewAng.y, viewOffset);

	float leanAngle = calculateLeanAngle(Ycurrent, player);

	const float leanRatio = GetLeanRatio(leanAngle);

	viewOffset.z = ((player->GetFlags() & FL_DUCKING) ? (VEC_DUCK_VIEW_SCALED(player).z) : VEC_VIEW_SCALED(player).z) - (neo_lean_fp_lower_eyes_scale.GetFloat() * leanRatio);

	player->m_vecLean = Vector(0, viewOffset.y, -(neo_lean_fp_lower_eyes_scale.GetFloat() * leanRatio));
	player->SetViewOffset(viewOffset);

	viewAng.z = leanAngle;
#ifdef CLIENT_DLL
	engine->SetViewAngles(viewAng);
#endif

#ifdef GAME_DLL
	SetSimulationTime(gpGlobals->curtime);
	SetNextThink(gpGlobals->curtime);
#endif

	if (leanAngle >= 0)
	{
		return leanAngle * neo_lean_tp_exaggerate_scale.GetFloat();
	}
	else
	{
		const float leftleanOverleanRatio = (neo_lean_yaw_peek_right_amount.GetFloat() / neo_lean_yaw_peek_left_amount.GetFloat());
		return leanAngle * neo_lean_tp_exaggerate_scale.GetFloat() * leftleanOverleanRatio;
	}
}

void CNEOPredictedViewModel::CalcViewModelView(CBasePlayer *pOwner,
	const Vector& eyePosition, const QAngle& eyeAngles)
{
	if (pOwner->GetObserverMode() == OBS_MODE_IN_EYE)
	{
		if (auto *pTargetPlayer = dynamic_cast<CNEO_Player *>(pOwner->GetObserverTarget());
				pTargetPlayer && !pTargetPlayer->IsObserver())
		{
			// NEO NOTE (nullsystem): 1st person mode pOwner = pTargetPlayer eye position
			// Take the target player's viewmodel FOV instead, otherwise it'll just look like it doesn't
			// change viewmodel FOV on 1st spectate
			return CalcViewModelView(pTargetPlayer, eyePosition, eyeAngles);
		}
	}

	// Is there a nicer way to do this?
	auto weapon = static_cast<CWeaponHL2MPBase*>(GetOwningWeapon());

	if (!weapon)
	{
		return;
	}

	CHL2MPSWeaponInfo data = weapon->GetHL2MPWpnData();

	Vector vForward, vRight, vUp, newPos, vOffset;
	QAngle newAng, angOffset;

	newAng = eyeAngles;
	newPos = eyePosition;

	AngleVectors(newAng, &vForward, &vRight, &vUp);

	if (auto neoPlayer = static_cast<CNEO_Player*>(pOwner))
	{
		vOffset = m_vOffset;
		angOffset = m_angOffset;
#ifdef CLIENT_DLL
		if (!prediction->InPrediction())
#endif
		{
			const bool playerAiming = neoPlayer->IsInAim();
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
			const float endAimingChange = m_flStartAimingChange + NEO_ZOOM_SPEED;
			const bool inAimingChange = (m_flStartAimingChange <= currentTime && currentTime < endAimingChange);
			if (inAimingChange)
			{
				float percentage = clamp((currentTime - m_flStartAimingChange) / NEO_ZOOM_SPEED, 0.0f, 1.0f);
				if (playerAiming) percentage = 1.0f - percentage;
				vOffset = Lerp(percentage, data.m_vecVMAimPosOffset, data.m_vecVMPosOffset);
				angOffset = Lerp(percentage, data.m_angVMAimAngOffset, data.m_angVMAngOffset);
			}
			else
			{
				vOffset = (playerAiming) ? data.m_vecVMAimPosOffset : data.m_vecVMPosOffset;
				angOffset = (playerAiming) ? data.m_angVMAimAngOffset : data.m_angVMAngOffset;
			}
			m_vOffset = vOffset;
			m_angOffset = angOffset;
		}
	}
	
	newPos += vForward * vOffset.x;
	newPos += vRight * vOffset.y;
	newPos += vUp * vOffset.z;

	newAng += angOffset;

	BaseClass::CalcViewModelView(pOwner, newPos, newAng);
}

#ifdef CLIENT_DLL
void CNEOPredictedViewModel::ProcessMuzzleFlashEvent()
{
	Vector vAttachment;
	QAngle dummyAngles;
	GetAttachment(2, vAttachment, dummyAngles);

	// Make a dlight
	dlight_t* dl = effects->CL_AllocDlight(LIGHT_INDEX_MUZZLEFLASH + index);
	dl->origin = vAttachment;
	dl->radius = random->RandomInt(64, 96);
	dl->decay = dl->radius / 0.1f;
	dl->die = gpGlobals->curtime + 0.1f;
	dl->color.r = 255;
	dl->color.g = 192;
	dl->color.b = 64;
	dl->color.exponent = 5;
}

RenderGroup_t CNEOPredictedViewModel::GetRenderGroup()
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());
	if (pPlayer)
	{
		return pPlayer->IsCloaked() ? RENDER_GROUP_VIEW_MODEL_TRANSLUCENT : RENDER_GROUP_VIEW_MODEL_OPAQUE;
	}

	return BaseClass::GetRenderGroup();
}

bool CNEOPredictedViewModel::UsesPowerOfTwoFrameBufferTexture()
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());
	if (pPlayer)
	{
		return pPlayer->IsCloaked() ? true : false;
	}

	return BaseClass::UsesPowerOfTwoFrameBufferTexture();
}
#endif
