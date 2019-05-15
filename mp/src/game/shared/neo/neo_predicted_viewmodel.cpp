#include "cbase.h"
#include "neo_predicted_viewmodel.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "prediction.h"
#include "ivieweffects.h"
#include "iinput.h"
#else
#include "neo_player.h"
#include "gameinterface.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( neo_predicted_viewmodel, CNEOPredictedViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( NEOPredictedViewModel, DT_NEOPredictedViewModel )

BEGIN_NETWORK_TABLE( CNEOPredictedViewModel, DT_NEOPredictedViewModel )
END_NETWORK_TABLE()

CNEOPredictedViewModel::CNEOPredictedViewModel()
{
    m_vecLeanDolly = vec3_origin;
    m_angLeanAngle = vec3_angle;
}

CNEOPredictedViewModel::~CNEOPredictedViewModel()
{
#ifdef CLIENT_DLL
    RemoveFromInterpolationList();
#endif
}

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

static inline void QNormalize(Quaternion &out)
{
    const double len = sqrt(
        out.x * out.x +
        out.y * out.y +
        out.z * out.z +
        out.w * out.w);
    
    out.x /= len;
    out.y /= len;
    out.z /= len;
    out.w /= len;
}

// Returns true if f1 and f2 are within the tolerance range of each other.
static inline bool RoughlyEquals(const float &f1, const float &f2, const float &tolerance = 0.1f)
{
    return fabs(f1 - f2) < tolerance;
}

static ConVar neo_lean_yaw_lerp_scale("neo_lean_yaw_lerp_scale", "0.075", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast to lerp viewoffset yaw into full lean position.");
static ConVar neo_lean_roll_lerp_scale("neo_lean_roll_lerp_scale", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast to lerp viewangoffset roll into full lean position.");
static ConVar neo_lean_yaw_peek_amount("neo_lean_yaw_peek_amount", "32.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full lean view reach.");
// Engine code starts to fight back at angles beyond 50, so we cap the max value there.
static ConVar neo_lean_angle("neo_lean_angle", "33.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Angle of a full lean.", true, 0.0, true, 50.0);

// NEO FIXME (Rain): we may gimbal lock!
void CNEOPredictedViewModel::CalcLean(CNEO_Player *player/*, float lastThink*/)
{
#ifdef CLIENT_DLL
    // We will touch view angles, so we have to resample here
    // to avoid disturbing user's mouse input.
    // This should also handle controller input.
    input->ExtraMouseSample(gpGlobals->absoluteframetime, 1);
#endif

    static Vector m_leanPosTargetOffset;
    const Vector viewOffset = player->GetViewOffset();

    QAngle eyeAngles = player->LocalEyeAngles();
    QAngle angOffset = eyeAngles;

    Vector viewDelta(0.0f, viewOffset.y, viewOffset.z);

    bool leaningIn = false;

    // If we're actively leaning
    if (player->m_nButtons & IN_LEAN_LEFT)
    {
        // Except if we're leaning both ways, in which case cancel
        if (player->m_nButtons & IN_LEAN_RIGHT)
        {
            leaningIn = false;
            angOffset.z = 0;
        }
        // Leaning exclusively left
        else
        {
            leaningIn = true;
            angOffset.z = -neo_lean_angle.GetFloat();

            viewDelta.y = neo_lean_yaw_peek_amount.GetFloat();
        }
    }
    // Leaning exclusively right
    else if (player->m_nButtons & IN_LEAN_RIGHT)
    {
        leaningIn = true;
        angOffset.z = neo_lean_angle.GetFloat();

        viewDelta.y = -neo_lean_yaw_peek_amount.GetFloat();
    }
    // Not leaning at all
    else
    {
        leaningIn = false;
        angOffset.z = 0;
    }

    // Same deal as above, but with view "dolly" pos
    if ((player->m_nButtons & IN_LEAN_LEFT) || (player->m_nButtons & IN_LEAN_RIGHT))
    {
        if ((player->m_nButtons & IN_LEAN_LEFT) && (player->m_nButtons & IN_LEAN_RIGHT))
        {
            m_leanPosTargetOffset = (player->m_nButtons & IN_DUCK) ? VEC_DUCK_VIEW : VEC_VIEW;
        }
        else
        {
            VectorYawRotate(viewDelta, player->GetLocalAngles().y, m_leanPosTargetOffset);
        }
    }
    else
    {
        m_leanPosTargetOffset = (player->m_nButtons & IN_DUCK) ? VEC_DUCK_VIEW : VEC_VIEW;
    }

    const float tolerance = 0.001;

    float angDrift = fabs(eyeAngles.z - angOffset.z);
    float posDrift_X = fabs(viewOffset.x - m_leanPosTargetOffset.x);
    float posDrift_Y = fabs(viewOffset.y - m_leanPosTargetOffset.y);

    bool wantAngLerp = (angDrift > tolerance);
    bool wantPosLerp = (posDrift_X > tolerance) || (posDrift_Y > tolerance);

#if(0)
    DevMsg("wantAng: %i wantPos: %i\n",
        (int)wantAngLerp, (int)wantAngLerp);
    
    DevMsg("Z comparison: %f -- %f\n", eyeAngles.z, angOffset.z);
    DevMsg("xyz comp: %f %f %f -- %f %f %f\n",
        viewOffset.x, viewOffset.y, viewOffset.z,
        m_leanPosTargetOffset.x, m_leanPosTargetOffset.y, m_leanPosTargetOffset.z);
#endif

    if (wantAngLerp || wantPosLerp)
    {
        const float angLerp = (leaningIn ?
            (neo_lean_roll_lerp_scale.GetFloat()) : (neo_lean_roll_lerp_scale.GetFloat() * 1.5));
        
        const float posLerp = (leaningIn ?
            (neo_lean_yaw_lerp_scale.GetFloat()) : (neo_lean_yaw_lerp_scale.GetFloat() * 1.5));

        NeoVmInterpolateAngles(eyeAngles, angOffset, m_angLeanAngle, angLerp);
        VectorLerp(viewOffset, m_leanPosTargetOffset, posLerp, m_vecLeanDolly);

        float angDrift = fabs(m_angLeanAngle.z - angOffset.z);
        float posDrift_X = fabs(viewOffset.x - m_leanPosTargetOffset.x);
        float posDrift_Y = fabs(viewOffset.y - m_leanPosTargetOffset.y);

        if (angDrift <= tolerance)
        {
            m_angLeanAngle.z = angOffset.z;
        }
        if (posDrift_X <= tolerance || posDrift_Y <= tolerance)
        {
            m_vecLeanDolly.x = m_leanPosTargetOffset.x;
            m_vecLeanDolly.y = m_leanPosTargetOffset.y;
        }

#ifdef CLIENT_DLL
        if (!prediction->InPrediction() && prediction->IsFirstTimePredicted())
#endif
        {
            // This provides us with lag compensated out values.
            CalcViewModelLag(m_vecLeanDolly, m_angLeanAngle, eyeAngles);
        }

#ifdef CLIENT_DLL
        // NOTE: we must sample mouse (input->ExtraMouseSample) before calling this,
        // otherwise we risk network view jitter when user turns their
        // view and applies the lean roll simultaneously!
        engine->SetViewAngles(m_angLeanAngle);
#endif
        player->SetViewOffset(m_vecLeanDolly);
    }
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