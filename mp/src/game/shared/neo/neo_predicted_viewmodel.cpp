#include "cbase.h"
#include "neo_predicted_viewmodel.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "prediction.h"
#include "iinput.h"
#include "engine/ivdebugoverlay.h"
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( neo_predicted_viewmodel, CNEOPredictedViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( NEOPredictedViewModel, DT_NEOPredictedViewModel )

BEGIN_NETWORK_TABLE( CNEOPredictedViewModel, DT_NEOPredictedViewModel )
END_NETWORK_TABLE()

CNEOPredictedViewModel::CNEOPredictedViewModel()
{
	m_vecNextViewOffset = vec3_origin;

	m_angNextViewAngles = vec3_angle;
}

CNEOPredictedViewModel::~CNEOPredictedViewModel()
{
#ifdef CLIENT_DLL
	RemoveFromInterpolationList();
#endif
}

// Interpolate euler angles via quaternions to avoid gimbal lock.
// This helper function is copied here from cdll_util, to have shared access.
static inline void NeoVmInterpolateAngles(const QAngle& start,
	const QAngle& end, QAngle& output, float frac)
{
	Quaternion src, dest;

	// Convert to quaternions
	AngleQuaternion( start, src );
	AngleQuaternion( end, dest );

	Quaternion result;

	// Slerp
	QuaternionSlerp( src, dest, frac, result );

	// Convert to euler
	QuaternionAngles( result, output );
}

#ifdef CLIENT_DLL
inline void CNEOPredictedViewModel::DrawRenderToTextureDebugInfo(IClientRenderable* pRenderable,
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
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 
	debugoverlay->AddLineOverlay( end2, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( start, vecSize.y, vec[1], end );
	VectorMA( end, vecSize.z, vec[2], end2 );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 
	debugoverlay->AddLineOverlay( end2, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( start, vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 );
	
	start = end;
	VectorMA( start, vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( start, vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( end, vecSize.x, vec[0], start );
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( start, -vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 );

	start = end;
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, rgbColor[0], rgbColor[1], rgbColor[2], true, 0.01 ); 

	C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
	int lineOffset = V_stristr("end", message) ? 1 : 0;
	if ( pEnt )
	{
		debugoverlay->AddTextOverlay( vecOrigin, lineOffset, "%s -- %d", message, pEnt->entindex() );
	}
	else
	{
		debugoverlay->AddTextOverlay( vecOrigin, lineOffset, "%s -- %X", message, (size_t)pRenderable );
	}
}
#endif

ConVar neo_lean_debug_draw_hull("neo_lean_debug_draw_hull", "0", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_hullmins_offset_x("neo_lean_hullmins_offset_x", "10", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_hullmins_offset_y("neo_lean_hullmins_offset_y", "2", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_hullmins_offset_z("neo_lean_hullmins_offset_z", "40", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_hullmaxs_offset_x("neo_lean_hullmaxs_offset_x", "-10", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_hullmaxs_offset_y("neo_lean_hullmaxs_offset_y", "-2", FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar neo_lean_hullmaxs_offset_z("neo_lean_hullmaxs_offset_z", "-1", FCVAR_CHEAT | FCVAR_REPLICATED);
static inline bool IsThereRoomForLeanSlide(CNEO_Player *player,
	const Vector &targetViewOffset, bool &outStartInSolid)
{
	//DevMsg("Targetviewoffset: %f %f %f\n", targetViewOffset[0], targetViewOffset[1], targetViewOffset[2]);
	const Vector startViewPos = player->GetAbsOrigin();
	const Vector endViewPos = startViewPos + Vector(
		targetViewOffset.x,
		targetViewOffset.y,
		0);

	// We can only lean through stuff that isn't solid for us
	CTraceFilterNoNPCsOrPlayer filter(player, COLLISION_GROUP_PLAYER_MOVEMENT);

	Vector hullMins = (player->m_nButtons & IN_DUCK) ?
		NEORules()->GetViewVectors()->m_vDuckHullMin :
		NEORules()->GetViewVectors()->m_vHullMin;
	
	Vector hullMaxs = (player->m_nButtons & IN_DUCK) ?
		NEORules()->GetViewVectors()->m_vDuckHullMax :
		NEORules()->GetViewVectors()->m_vHullMax;
	
	float multiplier;
	if (NEORules()->GetViewVectors()->m_vHullMax.z == 0)
	{
		Assert(false);
		multiplier = 1;
	}
	else
	{
		multiplier = player->m_nButtons & IN_DUCK ?
			NEORules()->GetViewVectors()->m_vDuckHullMax.z / NEORules()->GetViewVectors()->m_vHullMax.z
			: 1;
	}

	hullMins += Vector(
		neo_lean_hullmins_offset_x.GetFloat() * multiplier,
		neo_lean_hullmins_offset_y.GetFloat() * multiplier,
		neo_lean_hullmins_offset_z.GetFloat() * multiplier);
	hullMaxs += Vector(
		neo_lean_hullmaxs_offset_x.GetFloat() * multiplier,
		neo_lean_hullmaxs_offset_y.GetFloat() * multiplier,
		neo_lean_hullmaxs_offset_z.GetFloat() * multiplier);

	Ray_t ray;
	ray.Init(startViewPos, endViewPos, hullMins, hullMaxs);
	
	trace_t trace;
	enginetrace->TraceRay(ray, player->PhysicsSolidMaskForEntity(),
		&filter, &trace);

	if (trace.startsolid)
	{
		outStartInSolid = true;
		return false;
	}

	const float targetLen = startViewPos.AsVector2D().
		DistTo(endViewPos.AsVector2D());
	
	const float traceLen = trace.startpos.AsVector2D().
		DistTo(trace.endpos.AsVector2D());

	const float tolerance = 0.01f;
	const float delta = fabs(traceLen - targetLen);
	const bool weHaveRoom = (delta < tolerance);
	
#ifdef CLIENT_DLL
	if (neo_lean_debug_draw_hull.GetBool())
	{
		const Vector colorBlue(0, 0, 255), colorGreen(0, 255, 0), colorRed(255, 0, 0);
		player->GetNEOViewModel()->
			DrawRenderToTextureDebugInfo(player, hullMins, hullMaxs, colorBlue, "startLeanPos", startViewPos);
		player->GetNEOViewModel()->
			DrawRenderToTextureDebugInfo(player, hullMins, hullMaxs, (weHaveRoom ? colorGreen : colorRed), "endLeanPos", endViewPos);
#if(0)
		if (!weHaveRoom)
		{
			DevMsg("We didn't have room to lean; delta: %f tol: %f\n",
				delta, tolerance);
		}
#endif
	}
#endif

	return weHaveRoom;
}

extern ConVar neo_lean_yaw_lerp_scale("neo_lean_yaw_lerp_scale", "0.125", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast to lerp viewoffset yaw into full lean position.", true, 0.0, false, 2.0);
extern ConVar neo_lean_roll_lerp_scale("neo_lean_roll_lerp_scale", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast to lerp viewangoffset roll into full lean position.", true, 0.0, false, 2.0);
// Original Neotokyo with the latest leftlean fix uses 7 for leftlean and 15 for rightlean yaw slide.
extern ConVar neo_lean_yaw_peek_left_amount("neo_lean_yaw_peek_left_amount", "7.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full left lean view reach.", true, 0.0, false, 0);
extern ConVar neo_lean_yaw_peek_right_amount("neo_lean_yaw_peek_right_amount", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full right lean view reach.", true, 0.0, false, 0);
// Engine code starts to fight back at angles beyond 50, so we cap the max value there. Original NT value is 20.
extern ConVar neo_lean_angle("neo_lean_angle", "20.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Angle of a full lean.", true, 0.0, true, 50.0);

int CNEOPredictedViewModel::CalcLean(CNEO_Player *player)
{
#ifdef CLIENT_DLL
	// We will touch view angles, so we have to resample
	// here to avoid disturbing user's mouse look input.
	// This should also handle controller look input.
	// NEO TODO (Rain): Test if sampling works (no jitter) for gamepads.
	input->ExtraMouseSample(gpGlobals->absoluteframetime, 1);
#endif

	int leanDir = 0;

	//////////////////////////////////
	// Handle player lean key input //
	//////////////////////////////////

	bool leaningIn;
	float leanRotation, leanSideways;

	// If we're actively leaning left
	if (player->m_nButtons & IN_LEAN_LEFT)
	{
		// Except if we're leaning both ways, in which case cancel
		if (player->m_nButtons & IN_LEAN_RIGHT)
		{
			leaningIn = false;
			leanRotation = 0;
			leanSideways = 0;
		}
		// Leaning exclusively left
		else
		{
			leaningIn = true;
			leanRotation = -neo_lean_angle.GetFloat();
			leanSideways = neo_lean_yaw_peek_left_amount.GetFloat();

			leanDir = IN_LEAN_LEFT;
		}
	}
	// Leaning exclusively right
	else if (player->m_nButtons & IN_LEAN_RIGHT)
	{
		leaningIn = true;
		leanRotation = neo_lean_angle.GetFloat();
		leanSideways = -neo_lean_yaw_peek_right_amount.GetFloat();

		leanDir = IN_LEAN_RIGHT;
	}
	// Not leaning at all
	else
	{
		leaningIn = false;
		leanRotation = 0;
		leanSideways = 0;
	}


	//////////////////////////////
	// Get target lean rotation //
	//////////////////////////////
#ifdef CLIENT_DLL
	QAngle startAng = player->LocalEyeAngles();
#else
	QAngle startAng = player->EyeAngles();
#endif
	QAngle targetAng(startAng.x, startAng.y, leanRotation);

	/////////////////////////
	// Get target lean yaw //
	/////////////////////////

	const float verticalOffset = (player->m_nButtons & IN_DUCK) ? VEC_DUCK_VIEW.z : VEC_VIEW.z;
	Vector eyeOffset(0, leanSideways, verticalOffset);
	VectorYawRotate(eyeOffset, player->GetLocalAngles().y, eyeOffset);


	/////////////////////////////////////////////////
	// Calculate lerp values for the next position //
	/////////////////////////////////////////////////

	float rotationDiff = fabs(startAng.z - targetAng.z);
	float sidewaySlideDiff = fabs(eyeOffset.x - m_vecNextViewOffset.x);
	float forwardSlideDiff = fabs(eyeOffset.y - m_vecNextViewOffset.y);

	////////////////////////////////////////////
	// Lerp & assign the new eye pos & angles //
	////////////////////////////////////////////

#ifdef CLIENT_DLL
	const float thisTime = player->GetFinalPredictedTime() + TICK_INTERVAL -
		(gpGlobals->interpolation_amount * TICK_INTERVAL);
	static float lastTime = thisTime;
	const float dTime = thisTime - lastTime;
	lastTime = thisTime;
	//DevMsg("dtime: %f\n", dTime);
#endif

#ifdef CLIENT_DLL
	const float rotationLerp = neo_lean_roll_lerp_scale.GetFloat() * dTime;
	const float slideLerp = neo_lean_yaw_lerp_scale.GetFloat() * dTime;
#else
	const float rotationLerp = neo_lean_roll_lerp_scale.GetFloat();
	const float slideLerp = neo_lean_yaw_lerp_scale.GetFloat();
#endif

	// We allow this much lerp inaccuracy, and then snap to target location.
	// This prevents slight inaccuracy in final camera roll as we converge.
	const float tolerance = 0.05f;

	// Can we snap eye angles?
	if (rotationDiff < tolerance)
	{
		m_angNextViewAngles = targetAng;
	}
	else
	{
		// Interpolate eye angles
		NeoVmInterpolateAngles(startAng, targetAng, m_angNextViewAngles, rotationLerp);
	}

	// Can we smap yaw offset?
	if (sidewaySlideDiff < tolerance && forwardSlideDiff < tolerance)
	{
		m_vecNextViewOffset = eyeOffset;
	}
	else
	{
		// Interpolate eye position offset
		VectorLerp(player->GetViewOffset(), eyeOffset, slideLerp, m_vecNextViewOffset);
	}

	bool startInSolid = false;
	if (!IsThereRoomForLeanSlide(player, m_vecNextViewOffset, startInSolid))
	{
		// Always allow un-lean
		if (leaningIn)
		{
			// Zero the lean intent if we're clipping.
			//
			// NEO HACK/FIXME (Rain): Instead of doing a boolean lean trace,
			// we should see how many units of clearance we've got, and use that.
			// Current method will sometimes be too late to prevent view clipping,
			// and will lerp jitter on half-leans that collide.
			player->m_nButtons &= ~IN_LEAN_LEFT;
			player->m_nButtons &= ~IN_LEAN_RIGHT;

			if (startAng.z == 0 && eyeOffset.x == 0 && eyeOffset.y == 0)
			{
				return leanDir;
			}

			targetAng.z = 0;
			NeoVmInterpolateAngles(startAng, targetAng, m_angNextViewAngles,
				neo_lean_roll_lerp_scale.GetFloat() * 1.5);

			eyeOffset.x = 0;
			eyeOffset.y = 0;
			VectorLerp(player->GetViewOffset(), eyeOffset,
				neo_lean_yaw_lerp_scale.GetFloat() * 1.5, m_vecNextViewOffset);
		}
	}

	// See if we've reached the tolerance, and snap to target if so.
	// This prevents our view from drifting due to float inaccuracy.
	if (fabs(m_angNextViewAngles.z - targetAng.z) <= tolerance)
	{
		m_angNextViewAngles.z = targetAng.z;
	}

#ifdef CLIENT_DLL
	if (!prediction->InPrediction() && prediction->IsFirstTimePredicted())
#endif
	{
		// This provides us with lag compensated out values.
		CalcViewModelLag(m_vecNextViewOffset, m_angNextViewAngles, startAng);
	}

#ifdef CLIENT_DLL
	// NOTE: we must sample mouse input (input->ExtraMouseSample)
	// before calling this, otherwise we risk network view jitter
	// if the user turns their view and applies lean simultaneously!
	engine->SetViewAngles(m_angNextViewAngles);
#endif
	player->SetViewOffset(m_vecNextViewOffset);

#ifdef CLIENT_DLL
	if (player->ShouldInterpolate())
	{
		player->Interpolate(gpGlobals->curtime);
	}
#endif

	return leanDir;
}

#ifdef CLIENT_DLL
extern ConVar cl_wpn_sway_interp;
extern ConVar cl_wpn_sway_scale;
#endif
ConVar neo_lean_allow_extrapolation("neo_lean_allow_extrapolation", "1", FCVAR_REPLICATED | FCVAR_CHEAT,
	"Whether we're allowed to extrapolate lean beyond known values.", true, 0, true, 1);
void CNEOPredictedViewModel::CalcViewModelLag(Vector& origin, QAngle& angles,
	QAngle& original_angles)
{
	BaseClass::CalcViewModelLag(origin, angles, original_angles);
	return;

#ifdef CLIENT_DLL
	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );
	
	CInterpolationContext context;
	context.EnableExtrapolation( neo_lean_allow_extrapolation.GetBool() );

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );
	
	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
	Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= cl_wpn_sway_scale.GetFloat();
	origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;
#endif
}

void CNEOPredictedViewModel::CalcViewModelView(CBasePlayer *pOwner,
	const Vector& eyePosition, const QAngle& eyeAngles)
{
	//DevMsg("CNEOPredictedViewModel::CalcViewModelView: %f\n", eyeAngles.z);
	BaseClass::CalcViewModelView(pOwner, eyePosition, eyeAngles);
}