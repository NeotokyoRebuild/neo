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
#include "weapon_hl2mpbase.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel, CNEOPredictedViewModel);

IMPLEMENT_NETWORKCLASS_ALIASED( NEOPredictedViewModel, DT_NEOPredictedViewModel )

BEGIN_NETWORK_TABLE( CNEOPredictedViewModel, DT_NEOPredictedViewModel )
END_NETWORK_TABLE()

CNEOPredictedViewModel::CNEOPredictedViewModel()
{
	Yprevious = 0;
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
extern ConVar neo_lean_speed("neo_lean_speed", "120", FCVAR_REPLICATED | FCVAR_CHEAT, "Lean speed (units/second)", true, 0.0, false, 2.0);
// Original Neotokyo with the latest leftlean fix uses 7 for leftlean and 15 for rightlean yaw slide.
extern ConVar neo_lean_yaw_peek_left_amount("neo_lean_yaw_peek_left_amount", "7.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full left lean view reach.", true, 0.0, false, 0);
extern ConVar neo_lean_yaw_peek_right_amount("neo_lean_yaw_peek_right_amount", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full right lean view reach.", true, 0.0, false, 0);
extern ConVar neo_lean_angle_percentage("neo_lean_angle_percentage", "0.75", FCVAR_REPLICATED | FCVAR_CHEAT, "for adjusting the actual angle of lean to a percentage of lean.", true, 0.0, true, 1.0);

float CNEOPredictedViewModel::freeRoomForLean(float leanAmount, CNEO_Player *player){
	const Vector playerDefaultViewPos = player->GetAbsOrigin();
	Vector deltaPlayerViewPos(0, leanAmount, 0);
	VectorYawRotate(deltaPlayerViewPos, player->LocalEyeAngles().y, deltaPlayerViewPos);
	const Vector leanEndPos = playerDefaultViewPos + deltaPlayerViewPos;

	// We can only lean through stuff that isn't solid for us
	CTraceFilterNoNPCsOrPlayer filter(player, COLLISION_GROUP_PLAYER_MOVEMENT);

	Vector hullMins, hullMaxs;
	//sets player hull size
	//ducking
	Vector groundClearance(0, 0, 30);
	if (player->m_nButtons & IN_DUCK){
		hullMins = NEORules()->GetViewVectors()->m_vDuckHullMin + groundClearance;
		hullMaxs = NEORules()->GetViewVectors()->m_vDuckHullMax;
	}
	//standing
	else{
		hullMins = NEORules()->GetViewVectors()->m_vHullMin + groundClearance;
		hullMaxs = NEORules()->GetViewVectors()->m_vHullMax;
	}

	trace_t trace;
	UTIL_TraceHull(playerDefaultViewPos, leanEndPos, hullMins, hullMaxs, player->PhysicsSolidMaskForEntity(), &filter, &trace);
#ifdef CLIENT_DLL
	debugoverlay->AddTextOverlay(playerDefaultViewPos, 0.0001, "x: %f\n y: %f\n z:%f\n", playerDefaultViewPos.x, playerDefaultViewPos.y, playerDefaultViewPos.z);
	if (neo_lean_debug_draw_hull.GetBool()){
		const Vector colorBlue(0, 0, 255), colorGreen(0, 255, 0), colorRed(255, 0, 0);
		player->GetNEOViewModel()->DrawRenderToTextureDebugInfo(player, hullMins, hullMaxs, colorBlue, "startLeanPos", player->GetAbsOrigin());
	}
#endif

	return roundf(trace.startpos.DistTo(trace.endpos) * 100) / 100;
}




float CNEOPredictedViewModel::calculateLeanAngle(float freeRoom, CNEO_Player *player){
	float hipToHeadHeight = 41;
	return -RAD2DEG(atan2(freeRoom, hipToHeadHeight)) * neo_lean_angle_percentage.GetFloat();
}


void CNEOPredictedViewModel::lean(CNEO_Player *player){
#ifdef CLIENT_DLL
	input->ExtraMouseSample(gpGlobals->frametime,1);
#endif
	QAngle viewAng = player->LocalEyeAngles();
	float Ycurrent = Yprevious;
	float Yfinal = 0;

	auto leanButtons = player->m_nButtons;
	if (leanButtons & (IN_LEAN_LEFT | IN_LEAN_RIGHT)){
		if (leanButtons & IN_LEAN_LEFT & IN_LEAN_RIGHT){
			//leaning both ways
		}
		else if (leanButtons & IN_LEAN_LEFT){
			//leaning left
			Yfinal = freeRoomForLean(neo_lean_yaw_peek_left_amount.GetFloat(), player);
		}
		else{
			//leaning right
			Yfinal = -freeRoomForLean(-neo_lean_yaw_peek_right_amount.GetFloat(), player);
		}
	}
	else{
		//not leaning... move towards zero
	}

	float dY = Yfinal - Ycurrent;

	static double lastTime = gpGlobals->curtime;
	double thisTime = gpGlobals->curtime;
	const double dTime = thisTime - lastTime;
	lastTime = thisTime;
	float Ymoved = dTime * neo_lean_speed.GetFloat();

	if (dY > 0){
		Ycurrent += Ymoved;
		if (Ycurrent >= Yfinal - 0.05f){
			Ycurrent = Yfinal;
		}
	}
	else{
		Ycurrent -= Ymoved;
		if (Ycurrent <= Yfinal + 0.05f){
			Ycurrent = Yfinal;
		}
	}
	Vector viewOffset(0, 0, player->GetViewOffset().z);
	viewOffset.y = Ycurrent;
	Yprevious = Ycurrent;
	VectorYawRotate(viewOffset, viewAng.y, viewOffset);

	player->SetViewOffset(viewOffset);

	float leanAngle = calculateLeanAngle(Ycurrent, player);
	viewAng.z = leanAngle;
#ifdef CLIENT_DLL
	engine->SetViewAngles(viewAng);
#endif

}


#ifdef CLIENT_DLL
extern ConVar cl_wpn_sway_interp;
extern ConVar cl_wpn_sway_scale;
#endif
ConVar neo_lean_allow_extrapolation("neo_lean_allow_extrapolation", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Whether we're allowed to extrapolate lean beyond known values.", true, 0, true, 1);

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

void CNEOPredictedViewModel::SetWeaponModel(const char* pszModelname,
	CBaseCombatWeapon* weapon)
{
	BaseClass::SetWeaponModel(pszModelname, weapon);
}

