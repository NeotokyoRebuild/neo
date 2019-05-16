#include "cbase.h"
#include "neo_predicted_viewmodel.h"

#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "prediction.h"
#include "iinput.h"
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

static ConVar neo_lean_yaw_lerp_scale("neo_lean_yaw_lerp_scale", "0.075", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast to lerp viewoffset yaw into full lean position.");
static ConVar neo_lean_roll_lerp_scale("neo_lean_roll_lerp_scale", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast to lerp viewangoffset roll into full lean position.");
static ConVar neo_lean_yaw_peek_amount("neo_lean_yaw_peek_amount", "32.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How far sideways will a full lean view reach.");
// Engine code starts to fight back at angles beyond 50, so we cap the max value there.
static ConVar neo_lean_angle("neo_lean_angle", "33.0", FCVAR_REPLICATED | FCVAR_CHEAT, "Angle of a full lean.", true, 0.0, true, 50.0);

void CNEOPredictedViewModel::CalcLean(CNEO_Player *player)
{
#ifdef CLIENT_DLL
    // We will touch view angles, so we have to resample
    // here to avoid disturbing user's mouse look input.
    // This should also handle controller look input.
    input->ExtraMouseSample(gpGlobals->absoluteframetime, 1);
#endif


    //////////////////////////////////
    // Handle player lean key input //
    //////////////////////////////////

    bool leaningIn;
    float leanRotation, leanSideways;

    // If we're actively leaning
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
            leanSideways = neo_lean_yaw_peek_amount.GetFloat();
        }
    }
    // Leaning exclusively right
    else if (player->m_nButtons & IN_LEAN_RIGHT)
    {
        leaningIn = true;
        leanRotation = neo_lean_angle.GetFloat();
        leanSideways = -neo_lean_yaw_peek_amount.GetFloat();
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

    QAngle startAng = player->LocalEyeAngles();
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

    // We allow this much lerp inaccuracy, and then snap to target location.
    // This prevents slight inaccuracy in final camera roll as we converge.
    const float tolerance = 0.001f;

    float rotationDiff = fabs(startAng.z - targetAng.z);
    float sidewaySlideDiff = fabs(eyeOffset.x - m_vecNextViewOffset.x);
    float forwardSlideDiff = fabs(eyeOffset.y - m_vecNextViewOffset.y);

    bool wantRotationLerp = (rotationDiff > tolerance);
    bool wantSlideLerp = (sidewaySlideDiff > tolerance) || (forwardSlideDiff > tolerance);

    ////////////////////////////////////////////
    // Lerp & assign the new eye pos & angles //
    ////////////////////////////////////////////

    // We can skip this if we're within tolerance
    if (wantRotationLerp || wantSlideLerp)
    {
        float rotationLerp =
            (leaningIn ? (neo_lean_roll_lerp_scale.GetFloat()) : (neo_lean_roll_lerp_scale.GetFloat() * 1.5));
        
        float slideLerp =
            (leaningIn ? (neo_lean_yaw_lerp_scale.GetFloat()) : (neo_lean_yaw_lerp_scale.GetFloat() * 1.5));

        // Interpolate eye angles
        // NEO FIXME (Rain): This lerps at different rates based on ping.
        // This bug doesn't happen with viewpos lerping.
        NeoVmInterpolateAngles(startAng, targetAng, m_angNextViewAngles, rotationLerp);

        // Interpolate eye position offset
        VectorLerp(player->GetViewOffset(), eyeOffset, slideLerp, m_vecNextViewOffset);

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