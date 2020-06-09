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
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel, CNEOPredictedViewModel);

IMPLEMENT_NETWORKCLASS_ALIASED( NEOPredictedViewModel, DT_NEOPredictedViewModel )

BEGIN_NETWORK_TABLE( CNEOPredictedViewModel, DT_NEOPredictedViewModel )
END_NETWORK_TABLE()

CNEOPredictedViewModel::CNEOPredictedViewModel()
{
#ifdef CLIENT_DLL
#ifdef DEBUG
	IMaterial *pass = materials->FindMaterial("dev/toc_cloakpass", TEXTURE_GROUP_CLIENT_EFFECTS);
	Assert(pass && pass->IsPrecached());
#endif

	AddToInterpolationList();
#endif

	m_flYPrevious = 0;
	m_flLastLeanTime = 0;
}

CNEOPredictedViewModel::~CNEOPredictedViewModel()
{
#ifdef CLIENT_DLL
	RemoveFromInterpolationList();
#endif
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
ConVar neo_lean_yaw_peek_left_amount("neo_lean_yaw_peek_left_amount", "7.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full left lean view reach.", true, 0.0, false, 0);
ConVar neo_lean_yaw_peek_right_amount("neo_lean_yaw_peek_right_amount", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full right lean view reach.", true, 0.0, false, 0);
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

#define STAND_MINS (NEORules()->GetViewVectors()->m_vHullMin + groundClearance)
#define STAND_MAXS (NEORules()->GetViewVectors()->m_vHullMax)
#define DUCK_MINS (NEORules()->GetViewVectors()->m_vDuckHullMin + groundClearance)
#define DUCK_MAXS (NEORules()->GetViewVectors()->m_vDuckHullMax)

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
int CNEOPredictedViewModel::DrawModel(int flags)
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());

	Assert(pPlayer);

	if (pPlayer)
	{
		if (pPlayer->IsCloaked())
		{
			IMaterial *pass = materials->FindMaterial("dev/toc_vm", TEXTURE_GROUP_VIEW_MODEL);
			Assert(pass && !pass->IsErrorMaterial());

			if (pass && !pass->IsErrorMaterial())
			{
				if (!pass->IsPrecached())
				{
					PrecacheMaterial(pass->GetName());
					Assert(pass->IsPrecached());
				}

				//modelrender->SuppressEngineLighting(true);
				modelrender->ForcedMaterialOverride(pass);
				int ret = BaseClass::DrawModel(flags /*| STUDIO_RENDER | STUDIO_DRAWTRANSLUCENTSUBMODELS | STUDIO_TRANSPARENCY*/);
				//modelrender->SuppressEngineLighting(false);
				modelrender->ForcedMaterialOverride(NULL);
				return ret;
			}
			
			return 0;
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
	if (neo_lean_yaw_peek_left_amount.GetFloat() == 0 || neo_lean_yaw_peek_right_amount.GetFloat() == 0)
	{
		Assert(false);
		return 0.0f;
	}

	const float fullLean = (leanAngle < 0) ? neo_lean_yaw_peek_left_amount.GetFloat() : neo_lean_yaw_peek_right_amount.GetFloat();
	Assert(fullLean != 0);

	return fabs(leanAngle) / fullLean;
}

float CNEOPredictedViewModel::lean(CNEO_Player *player){
	Assert(player);
#ifdef CLIENT_DLL
	input->ExtraMouseSample(gpGlobals->frametime, 1);
#endif
	QAngle viewAng = player->LocalEyeAngles();
	float Ycurrent = m_flYPrevious;
	float Yfinal = 0;

	const int leanButtons = player->m_nButtons;
	if (player->IsAlive())
	{
		if ((leanButtons & IN_LEAN_LEFT) && !(leanButtons & IN_LEAN_RIGHT)) {
			//leaning left
			Yfinal = freeRoomForLean(neo_lean_yaw_peek_left_amount.GetFloat(), player);
		}
		else if ((leanButtons & IN_LEAN_RIGHT) && !(leanButtons & IN_LEAN_LEFT)) {
			//leaning right
			Yfinal = -freeRoomForLean(-neo_lean_yaw_peek_right_amount.GetFloat(), player);
		}
		else {
			//not leaning, or leaning both ways; move towards zero
		}
	}

	const float dY = Yfinal - Ycurrent;

	if (dY != 0){
		const float leanStep = 0.25f;

		// Almost done leaning; snap to zero to avoid back-and-forth "wiggle"
		if (fabs(dY) - leanStep < 0) {
			Ycurrent = Yfinal;
		}
		else {
#ifdef CLIENT_DLL
			// Less than 0.1 ms latency, this must be a LAN connection. Don't interpolate.
			if (engine->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING) < 0.0001) {
				Ycurrent = Lerp(leanStep * neo_lean_speed.GetFloat() * 1.5f, Ycurrent, Yfinal);
			}
			// We have to interpolate here to avoid prediction error jitter over network connections.
			else {
				Ycurrent = Lerp(leanStep * neo_lean_speed.GetFloat() * gpGlobals->interpolation_amount, Ycurrent, Yfinal);
			}
#else
			Ycurrent = Lerp(leanStep * neo_lean_speed.GetFloat(), Ycurrent, Yfinal);
#endif
		}
	}

	Vector viewOffset(0, 0, 0);
	viewOffset.y = m_flYPrevious = Ycurrent;

	VectorYawRotate(viewOffset, viewAng.y, viewOffset);

	float leanAngle = calculateLeanAngle(Ycurrent, player);

	const float leanRatio = GetLeanRatio(leanAngle);

	switch (player->GetClass())
	{
	case NEO_CLASS_RECON:
		viewOffset.z = ((player->GetFlags() & FL_DUCKING) ? NEO_RECON_EYE_HEIGHT_DUCKING : NEO_RECON_EYE_HEIGHT_STANDING) - (neo_lean_fp_lower_eyes_scale.GetFloat() * leanRatio);
		break;
	case NEO_CLASS_ASSAULT:
		viewOffset.z = ((player->GetFlags() & FL_DUCKING) ? NEO_ASSAULT_EYE_HEIGHT_DUCKING : NEO_ASSAULT_EYE_HEIGHT_STANDING) - (neo_lean_fp_lower_eyes_scale.GetFloat() * leanRatio);
		break;
	case NEO_CLASS_SUPPORT:
		viewOffset.z = ((player->GetFlags() & FL_DUCKING) ? NEO_SUPPORT_EYE_HEIGHT_DUCKING : NEO_SUPPORT_EYE_HEIGHT_STANDING) - (neo_lean_fp_lower_eyes_scale.GetFloat() * leanRatio);
		break;
	default:
		Assert(false);
		viewOffset.z = player->GetViewOffset().z;
		break;
	}

	player->SetViewOffset(viewOffset);

	viewAng.z = leanAngle;
#ifdef CLIENT_DLL
	engine->SetViewAngles(viewAng);
#endif

	if (leanAngle >= 0)
	{
		return leanAngle * neo_lean_tp_exaggerate_scale.GetFloat();
	}
	else
	{
		const float leftleanOverleanRatio = (neo_lean_yaw_peek_left_amount.GetFloat() == 0) ? 1.0f : (neo_lean_yaw_peek_right_amount.GetFloat() / neo_lean_yaw_peek_left_amount.GetFloat());
		return leanAngle * neo_lean_tp_exaggerate_scale.GetFloat() * leftleanOverleanRatio;
	}
}

void CNEOPredictedViewModel::CalcViewModelView(CBasePlayer *pOwner,
	const Vector& eyePosition, const QAngle& eyeAngles)
{
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

	vOffset = data.m_vecVMPosOffset;
	angOffset = data.m_angVMAngOffset;

	newPos += vForward * vOffset.x;
	newPos += vRight * vOffset.y;
	newPos += vUp * vOffset.z;

	newAng += angOffset;

	BaseClass::CalcViewModelView(pOwner, newPos, newAng);
}

#ifdef CLIENT_DLL
RenderGroup_t CNEOPredictedViewModel::GetRenderGroup()
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());
	Assert(pPlayer);

	if (pPlayer)
	{
		return pPlayer->IsCloaked() ? RENDER_GROUP_VIEW_MODEL_TRANSLUCENT : RENDER_GROUP_VIEW_MODEL_OPAQUE;
	}

	return BaseClass::GetRenderGroup();
}
#endif
