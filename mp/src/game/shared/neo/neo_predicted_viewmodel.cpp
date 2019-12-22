#include "cbase.h"
#include "neo_predicted_viewmodel.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "prediction.h"
#include "iinput.h"
#include "engine/ivdebugoverlay.h"

#include "viewrender.h"

#else
#include "neo_player.h"
#endif

#ifdef CLIENT_DLL
#include "model_types.h"
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
#ifdef CLIENT_DLL
#ifdef DEBUG
	IMaterial *pass = materials->FindMaterial("dev/toc_cloakpass", TEXTURE_GROUP_CLIENT_EFFECTS);
	Assert(pass && pass->IsPrecached());
#endif

	AddToInterpolationList();
#endif
}

CNEOPredictedViewModel::~CNEOPredictedViewModel()
{
#ifdef CLIENT_DLL
	RemoveFromInterpolationList();
#endif
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
ConVar neo_lean_speed("neo_lean_speed", "120", FCVAR_REPLICATED | FCVAR_CHEAT, "Lean speed (units/second)", true, 0.0, false, 2.0);
// Original Neotokyo with the latest leftlean fix uses 7 for leftlean and 15 for rightlean yaw slide.
ConVar neo_lean_yaw_peek_left_amount("neo_lean_yaw_peek_left_amount", "7.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full left lean view reach.", true, 0.0, false, 0);
ConVar neo_lean_yaw_peek_right_amount("neo_lean_yaw_peek_right_amount", "15.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full right lean view reach.", true, 0.0, false, 0);
ConVar neo_lean_angle_percentage("neo_lean_angle_percentage", "0.75", FCVAR_REPLICATED | FCVAR_CHEAT, "for adjusting the actual angle of lean to a percentage of lean.", true, 0.0, true, 1.0);

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
	if (neo_lean_debug_draw_hull.GetBool()){
		// Z offset to avoid text overlap; DrawRenderToTextureDebugInfo will also want to print text around similar area.
		const Vector debugOffset = Vector(playerDefaultViewPos) + Vector(0, 0, -4);
		debugoverlay->AddTextOverlay(debugOffset, 0.0001, "x: %f\n y: %f\n z:%f\n", playerDefaultViewPos.x, playerDefaultViewPos.y, playerDefaultViewPos.z);
		const Vector colorBlue(0, 0, 255), colorGreen(0, 255, 0), colorRed(255, 0, 0);
		player->GetNEOViewModel()->DrawRenderToTextureDebugInfo(player, hullMins, hullMaxs, colorBlue, "startLeanPos", player->GetAbsOrigin());
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

float CNEOPredictedViewModel::calculateLeanAngle(float freeRoom, CNEO_Player *player){
	float hipToHeadHeight = 41;
	return -RAD2DEG(atan2(freeRoom, hipToHeadHeight)) * neo_lean_angle_percentage.GetFloat();
}

void CNEOPredictedViewModel::lean(CNEO_Player *player){
#ifdef CLIENT_DLL
	input->ExtraMouseSample(gpGlobals->frametime, 1);
#endif
	QAngle viewAng = player->LocalEyeAngles();
	static float Yprevious = 0;
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
	if (dY != 0){
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
