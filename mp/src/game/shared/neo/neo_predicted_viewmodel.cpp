#include "cbase.h"
#include "neo_predicted_viewmodel.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "prediction.h"
#include "ivieweffects.h"
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

void CNEOPredictedViewModel::CalcLean(CNEO_Player *player/*, float lastThink*/)
{
    const float yawPeekAmount = 32.0f;
    const float lerpScale = 0.2f;

    static float leanStartTime = gpGlobals->curtime, leanEndTime = gpGlobals->curtime;

    static bool m_bInLeanLeft = false, m_bInLeanRight = false;

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
            angOffset.z = ((CNEORules*)g_pGameRules)->GetNEOViewVectors()->m_vViewAngLeanLeft.x;

            viewDelta.y = yawPeekAmount;
        }
    }
    // Leaning exclusively right
    else if (player->m_nButtons & IN_LEAN_RIGHT)
    {
        leaningIn = true;
        angOffset.z = ((CNEORules*)g_pGameRules)->GetNEOViewVectors()->m_vViewAngLeanRight.x;

        viewDelta.y = -yawPeekAmount;
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

    const float tolerance = 0.1;

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
        const float lerp = (leaningIn ? (lerpScale) : (lerpScale * 1.5));

        NeoVmInterpolateAngles(eyeAngles, angOffset, m_angLeanAngle, lerp);
        VectorLerp(viewOffset, m_leanPosTargetOffset, lerp, m_vecLeanDolly);

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
        if (false && !prediction->InPrediction() && prediction->IsFirstTimePredicted())
#endif
        {
            // This provides us with lag compensated out values.
            CalcViewModelLag(m_vecLeanDolly, m_angLeanAngle, eyeAngles);
        }

#ifdef CLIENT_DLL
        // NEO FIXME / BUG (Rain): This jitters clientside when ping > 0.
        // Do we need to inform server of the new angles?
        engine->SetViewAngles(m_angLeanAngle);
#endif
        player->SetViewOffset(m_vecLeanDolly);
    }
}

void CNEOPredictedViewModel::CalcViewModelView(CBasePlayer *pOwner,
    const Vector& eyePosition, const QAngle& eyeAngles)
{
    //DevMsg("CNEOPredictedViewModel::CalcViewModelView: %f\n", eyeAngles.z);
    BaseClass::CalcViewModelView(pOwner, eyePosition, eyeAngles);
}