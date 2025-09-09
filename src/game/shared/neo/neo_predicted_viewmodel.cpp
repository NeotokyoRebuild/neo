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
#include "in_main.h"
#else
#include "neo_player.h"
#include "bot/neo_bot.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED(NEOPredictedViewModel, DT_NEOPredictedViewModel)

BEGIN_NETWORK_TABLE(CNEOPredictedViewModel, DT_NEOPredictedViewModel)
#ifdef CLIENT_DLL
RecvPropFloat(RECVINFO(m_flLeanRatio)),
#else
SendPropFloat(SENDINFO(m_flLeanRatio)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOPredictedViewModel)
DEFINE_PRED_FIELD(m_flLeanRatio, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel, CNEOPredictedViewModel);

CNEOPredictedViewModel::CNEOPredictedViewModel()
#ifdef CLIENT_DLL
	: m_iv_flLeanRatio("CNEOPredictedViewModel::m_iv_flLeanRatio")
#endif
{
#ifdef CLIENT_DLL
#ifdef DEBUG
	IMaterial *pass = materials->FindMaterial("dev/toc_cloakpass", TEXTURE_GROUP_CLIENT_EFFECTS);
	Assert(pass && pass->IsPrecached());
#endif

	AddVar(&m_flLeanRatio, &m_iv_flLeanRatio, LATCH_SIMULATION_VAR);
#endif

	m_flLeanRatio = 0;
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
ConVar neo_lean_speed("neo_lean_speed", "0.2", FCVAR_REPLICATED | FCVAR_CHEAT, "Time to reach full lean", true, 0.0, false, 0); // 0.15 in OGNT, which feels too fast with easing
ConVar neo_lean_peek_left_amount("neo_lean_peek_left_amount", "7.5", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full left lean view reach.", true, 0.001f, false, 0); // 7.5 in OGNT
ConVar neo_lean_peek_right_amount("neo_lean_peek_right_amount", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full right lean view reach.", true, 0.001f, false, 0); // 15 in OGNT
ConVar neo_lean_fp_angle("neo_lean_fp_angle", "20", FCVAR_REPLICATED | FCVAR_CHEAT, "How many degrees does the camera lean.", true, 0.0, true, 45.0); // 20 in OGNT
ConVar neo_lean_tp_angle("neo_lean_tp_angle", "35", FCVAR_REPLICATED | FCVAR_CHEAT, "How many degrees does the player character lean.", true, 0.0, true, 45.0); // 35 in OGNT
ConVar neo_lean_fp_lower_eyes("neo_lean_fp_lower_eyes", "2", FCVAR_REPLICATED | FCVAR_CHEAT, "How low to bring eye-level when leaning.", true, 0.0, false, 0); // 0 in OGNT

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

	Vector hullModifier = Vector(0.75, 0.75, 1);
#if(0) // same view limits regardless of player class
#define STAND_MINS (NEORules()->GetViewVectors()->m_vHullMin + groundClearance)
#define STAND_MAXS (NEORules()->GetViewVectors()->m_vHullMax)
#define DUCK_MINS (NEORules()->GetViewVectors()->m_vDuckHullMin + groundClearance)
#define DUCK_MAXS (NEORules()->GetViewVectors()->m_vDuckHullMax)
#else // class hull specific limits
#define STAND_MINS ((VEC_HULL_MIN_SCALED(player) * hullModifier) + groundClearance)
#define STAND_MAXS (VEC_HULL_MAX_SCALED(player) * hullModifier)
#define DUCK_MINS ((VEC_DUCK_HULL_MIN_SCALED(player) * hullModifier) + groundClearance)
#define DUCK_MAXS (VEC_DUCK_HULL_MAX_SCALED(player) * hullModifier)
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
extern ConVar glow_outline_effect_enable;
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
		if (pPlayer->GetClass() == NEO_CLASS_SUPPORT && pPlayer->IsInVision() && !glow_outline_effect_enable.GetBool())
		{
			IMaterial* pass = materials->FindMaterial("dev/thermal_view_model", TEXTURE_GROUP_MODEL);
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

float EaseOut(float current, float target, float step)
{
	float start = target > current ? -1.0f : 1.0f;
	// Scale depeding on how far we're leaning and multiply by
	// 0.5 / sqrt(0.5), because we want lean_speed to be the time
	// from the center to an edge, which after remapping is at 0.5.
	float stepScale = fabs(target - start) * 0.7071067811865475f;
	float remapped = sqrtf(RemapVal(current, start, target, 1.0f, 0.0f));
	remapped -= step / stepScale;
	if (remapped < 0.0f)
	{
		remapped = 0.0f;
	}
	return  RemapVal(remapped * remapped, 1.0f, 0.0f, start, target);
}

enum NeoAutoLean
{
	NEO_AUTOLEAN_OFF = 0, 		// Never auto lean
	NEO_AUTOLEAN_AIM,			// Only auto lean on aim
	NEO_AUTOLEAN_AIM_NORMSPEED,	// Auto lean on aim and normal speed/not sprinting

	NEO_AUTOLEAN__TOTAL,
	NEO_AUTOLEAN__MAXVAL = NEO_AUTOLEAN__TOTAL - 1,
};

#ifdef CLIENT_DLL
ConVar cl_neo_lean_viewmodel_only("cl_neo_lean_viewmodel_only", "0", FCVAR_ARCHIVE, "Rotate view-model instead of camera when leaning", true, 0, true, 1);
ConVar cl_neo_lean_automatic("cl_neo_lean_automatic", "0", FCVAR_ARCHIVE, "Automatic leaning around corners", true, 0, true, NEO_AUTOLEAN__MAXVAL);
#ifdef DEBUG
ConVar cl_neo_lean_automatic_debug("cl_neo_lean_automatic_debug", "0", FCVAR_ARCHIVE | FCVAR_DEVELOPMENTONLY, "Show automatic leaning tracelines", true, 0, true, 1);
#endif // DEBUG
#endif // CLIENT_DLL

float CNEOPredictedViewModel::lean(CNEO_Player *player){
	Assert(player);
#ifdef CLIENT_DLL
	input->ExtraMouseSample(gpGlobals->frametime, 1);
#endif
	QAngle viewAng = player->LocalEyeAngles();
	float leanRatio = 0;

	if (player->IsAlive() && player->GetFlags() & FL_ONGROUND)
	{
#ifdef GAME_DLL
		if (player->IsBot() && !(player->GetNeoFlags() & NEO_FL_FREEZETIME))
#endif
		{
#ifdef CLIENT_DLL
			int cl_neo_lean_automatic_value = cl_neo_lean_automatic.GetInt();
#else
			const int cl_neo_lean_automatic_value = (player->GetClass() == NEO_CLASS_SUPPORT) ? NEO_AUTOLEAN_AIM : NEO_AUTOLEAN_AIM_NORMSPEED;
#endif
			if (cl_neo_lean_automatic_value)
			{
				int total = 0;
				if (cl_neo_lean_automatic_value == NEO_AUTOLEAN_AIM_NORMSPEED || (cl_neo_lean_automatic_value == NEO_AUTOLEAN_AIM && player->IsInAim()))
				{
					Vector startPos = player->GetAbsOrigin();
					startPos.z = player->EyePosition().z; // Ideally shouldn't move due to lean but this is a nice spot
					constexpr float tracelineAngleChange = 15.f;
					constexpr int startingDistance = 80;
					int distance = startingDistance;
					trace_t tr;
					for (int i = 1; i <= 10; i++)
					{
						Vector endPos = startPos;
						float offset = (i / (tracelineAngleChange - i));
						endPos.x += cos(DEG2RAD(viewAng.y) + offset) * distance;
						endPos.y += sin(DEG2RAD(viewAng.y) + offset) * distance;
						UTIL_TraceLine(startPos, endPos, MASK_SHOT_HULL, player, COLLISION_GROUP_NONE, &tr);
						if (tr.fraction != 1.0)
						{
							total -= 1;
						}
						distance = Max(20, distance - 5);
#if defined(CLIENT_DLL) && defined(DEBUG)
						if (cl_neo_lean_automatic_debug.GetBool())
						{
							DebugDrawLine(startPos, endPos, 0, tr.fraction == 1.0 ? 255 : 0, 255, 0, 0);
						}
#endif // defined(CLIENT_DLL) && defined(DEBUG)
					}
					distance = startingDistance;
					for (int i = -1; i >= -10; i--)
					{
						Vector endPos = startPos;
						float offset = (i / (tracelineAngleChange + i));
						endPos.x += cos(DEG2RAD(viewAng.y) + offset) * distance;
						endPos.y += sin(DEG2RAD(viewAng.y) + offset) * distance;
						UTIL_TraceLine(startPos, endPos, MASK_SHOT_HULL, player, COLLISION_GROUP_NONE, &tr);
						if (tr.fraction != 1.0)
						{
							total += 1;
						}
						distance = Max(20, distance - 5);
#if defined(CLIENT_DLL) && defined(DEBUG)
						if (cl_neo_lean_automatic_debug.GetBool())
						{
							DebugDrawLine(startPos, endPos, 255, tr.fraction == 1.0 ? 255 : 0, 0, 0, 0);
						}
#endif // defined(CLIENT_DLL) && defined(DEBUG)
					}
				}
#ifdef CLIENT_DLL
				if (total < 0)	{ IN_LeanRight(); }
				else if (total > 0) { IN_LeanLeft(); }
				else { IN_LeanReset(); }
#else
				if (auto *botPlayer = dynamic_cast<CNEOBot *>(player))
				{
					if (total < 0)
					{
						botPlayer->ReleaseLeanLeftButton();
						botPlayer->PressLeanRightButton();
					}
					else if (total > 0)
					{
						botPlayer->ReleaseLeanRightButton();
						botPlayer->PressLeanLeftButton();
					}
					else
					{
						botPlayer->ReleaseLeanLeftButton();
						botPlayer->ReleaseLeanRightButton();
					}
				}
#endif
			}
		}

		switch (player->m_bInLean.Get())
		{
		case NEO_LEAN_LEFT:
			leanRatio = freeRoomForLean(neo_lean_peek_left_amount.GetFloat(), player) / neo_lean_peek_left_amount.GetFloat();
			break;
		case NEO_LEAN_RIGHT:
			leanRatio = -freeRoomForLean(-neo_lean_peek_right_amount.GetFloat(), player) / neo_lean_peek_right_amount.GetFloat();
			break;
		default:
			// not leaning, or leaning both ways; move towards zero
			break;
		}
	}

	const float diff = leanRatio - m_flLeanRatio;

	if (diff != 0)
	{
		const float leanStep = 1 / neo_lean_speed.GetFloat() * gpGlobals->frametime;
		m_flLeanRatio = EaseOut(m_flLeanRatio, leanRatio, leanStep);
	}

	Vector viewOffset(0, 0, 0);
	viewOffset.y = m_flLeanRatio * (m_flLeanRatio < 0 ? neo_lean_peek_right_amount.GetFloat() : neo_lean_peek_left_amount.GetFloat());
	VectorYawRotate(viewOffset, viewAng.y, viewOffset);

	const float absLeanRatio = fabs(m_flLeanRatio);
	viewOffset.z = ((player->GetFlags() & FL_DUCKING) ? (VEC_DUCK_VIEW_SCALED(player).z) : VEC_VIEW_SCALED(player).z) - (neo_lean_fp_lower_eyes.GetFloat() * absLeanRatio);

	player->m_vecLean = Vector(0, viewOffset.y, -(neo_lean_fp_lower_eyes.GetFloat() * absLeanRatio));
	player->SetViewOffset(viewOffset);

#ifdef GAME_DLL
	if (player->IsBot())
#endif
	{
		viewAng.z = AngleNormalize(-m_flLeanRatio * ((player->GetClass() == NEO_CLASS_JUGGERNAUT) ? 15.0f : neo_lean_fp_angle.GetFloat()));
#ifdef CLIENT_DLL
		engine->SetViewAngles(viewAng);
#else
		player->PlayerData()->v_angle = viewAng;
#endif
	}

#ifdef GAME_DLL
	SetSimulationTime(gpGlobals->curtime);
	SetNextThink(gpGlobals->curtime);
#endif

	return -m_flLeanRatio * ((player->GetClass() == NEO_CLASS_JUGGERNAUT) ? 20.0f : neo_lean_tp_angle.GetFloat());
}

extern ConVar cl_righthand;
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

#ifdef CLIENT_DLL
			if (cl_neo_lean_viewmodel_only.GetBool())
			{ // extra viewmodel offset for when gun rotation would obstruct center of the screen. Should probably be done on a per weapon basis instead
				float percent = m_iv_flLeanRatio.GetCurrent();
				const bool rightHand = cl_righthand.GetBool();
				if ((rightHand && percent < 0) || (!rightHand && percent > 0))
				{
					percent = abs(percent);
					constexpr float FINAL_Y_EXTRA_OFFSET = 3;
					constexpr float FINAL_Z_EXTRA_OFFSET = 1;
					vOffset.y += FINAL_Y_EXTRA_OFFSET * percent;
					vOffset.z -= FINAL_Z_EXTRA_OFFSET * percent;
				}
			}
#endif // CLIENT_DLL
			m_vOffset = vOffset;
			m_angOffset = angOffset;
		}

		constexpr int VIEWMODEL_MOVE_DISTANCE = 24; // max distance viewmodel can move is actually VIEWMODEL_MOVE_DISTANCE - HALF_HULL_SIZE
		constexpr int HALF_HULL_SIZE = 12; // is this defined globally somewhere?
		constexpr float VIEWMODEL_MOVE_FRACTION_MIN = (VIEWMODEL_MOVE_DISTANCE - HALF_HULL_SIZE) / VIEWMODEL_MOVE_DISTANCE;
		if (m_flGunPushLastChangeTime < gpGlobals->curtime) // Why is this value sometimes greater than current time? Also this fixes gun moving back much slower around some corners, maybe weird prediction stuff?
		{
			constexpr float MAX_GUN_PUSH_FRACTION_CHANGE_PER_SECOND = 2.5f;
			float maxFractionChange = (gpGlobals->curtime - m_flGunPushLastChangeTime) * MAX_GUN_PUSH_FRACTION_CHANGE_PER_SECOND;
			m_flGunPushLastChangeTime = gpGlobals->curtime;

			trace_t tr;
			Vector startPos = pOwner->EyePosition();
			Vector endPos = pOwner->EyePosition() + (vForward * VIEWMODEL_MOVE_DISTANCE);
			// NEO NOTE (Adam) the hull generated by tracehull is fixed to the axis, using a non square cube shape will give varying results depending on player orientation to the obstacle in front of them (already the case to an extent since not using a sphere)
			UTIL_TraceHull(startPos, endPos, Vector(-4,-4,-4), Vector(4,4,4), MASK_SOLID &~ CONTENTS_MONSTER, pOwner, COLLISION_GROUP_NONE, &tr);
			tr.fraction = max(VIEWMODEL_MOVE_FRACTION_MIN, tr.fraction); // The smallest value tr.fraction can have given the size of the player hull
		
			if (abs(tr.fraction - m_flGunPush) < (maxFractionChange + 0.01)) // float math, don't want the gun to move when in its resting position
			{
				m_flGunPush = tr.fraction;
			}
			else
			{
				m_flGunPush += tr.fraction > m_flGunPush ? maxFractionChange : -maxFractionChange;
			}

		}
		Vector finalGunPush = vForward * ((1 - m_flGunPush) * VIEWMODEL_MOVE_DISTANCE);

		newPos += (vForward * vOffset.x) - finalGunPush;
		newPos += vRight * vOffset.y;
		newPos += vUp * vOffset.z;
	}

	newAng += angOffset;
#ifdef CLIENT_DLL
	if (cl_neo_lean_viewmodel_only.GetBool())
	{
		QAngle angles = pOwner->EyeAngles();
		newAng.z += cl_righthand.GetBool() ? angles.z : -angles.z;
	}
#endif

	BaseClass::CalcViewModelView(pOwner, newPos, newAng);
}

#ifdef CLIENT_DLL
void CNEOPredictedViewModel::ProcessMuzzleFlashEvent()
{
	Vector vAttachment;
	QAngle dummyAngles;
	GetAttachment(1, vAttachment, dummyAngles);

	// Make a dlight
	dlight_t* dl = effects->CL_AllocDlight(LIGHT_INDEX_MUZZLEFLASH + index);
	dl->origin = vAttachment;
	dl->radius = random->RandomInt(64, 96);
	dl->decay = dl->radius / 0.05f;
	dl->die = gpGlobals->curtime + 0.05f;
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
