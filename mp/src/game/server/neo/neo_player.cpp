#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "neo_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "hl2mp_player_shared.h"
#include "neo_predicted_viewmodel.h"
#include "in_buttons.h"
#include "neo_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"
#include "gamestats.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "ilagcompensationmanager.h"

#include "neo_model_manager.h"

#include "shareddefs.h"
#include "inetchannelinfo.h"
#include "eiface.h"

void DropPrimedFragGrenade(CNEO_Player *pPlayer, CBaseCombatWeapon *pGrenade);

LINK_ENTITY_TO_CLASS(player, CNEO_Player);

/*LINK_ENTITY_TO_CLASS(info_player_attacker, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_defender, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_start, CPointEntity);*/

IMPLEMENT_SERVERCLASS_ST(CNEO_Player, DT_NEO_Player)
    SendPropVector(SENDINFO(m_leanPos)),
    SendPropQAngles(SENDINFO(m_leanAng)),
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Player)
    DEFINE_FIELD(m_leanPos, FIELD_VECTOR),
    DEFINE_FIELD(m_leanAng, FIELD_VECTOR),
END_DATADESC()

CNEO_Player::CNEO_Player()
{
    m_leanAng.Set(QAngle(0, 0, 0));
    m_leanPos.Set(Vector(0, 0, 0));

    m_bInLeanLeft = false;
    m_bInLeanRight = false;

    m_leanPosTargetOffset = vec3_origin;

    SetPredictionEligible(true);
}

CNEO_Player::~CNEO_Player( void )
{

}

void CNEO_Player::Precache( void )
{
	BaseClass::Precache();
}

void CNEO_Player::Spawn(void)
{
    BaseClass::Spawn();

    ShowCrosshair(false);

    m_leanPosTargetOffset = VEC_VIEW;

#if(0)
    SetNextThink(gpGlobals->curtime);
#endif
}

void CNEO_Player::PreThink(void)
{
    BaseClass::PreThink();

    CNEOPredictedViewModel *vm = (CNEOPredictedViewModel*)GetViewModel();
    if (vm)
    {
        //static float lastThink = gpGlobals->curtime;
        vm->CalcLean(this/*, lastThink*/);
        //lastThink = gpGlobals->curtime;
    }
    else
    {
        Warning("CNEO_Player::PreThink: Failed to get CNEOPredictedViewModel\n");
    }
}

void CNEO_Player::PostThink(void)
{
    BaseClass::PostThink();

    CBaseCombatWeapon *pWep = GetActiveWeapon();

    if (pWep)
    {
        static bool previouslyReloading = false;

        if (pWep->m_bInReload)
        {
            if (!previouslyReloading)
            {
                Weapon_SetZoom(false);
            }
        }
        else
        {
            if (m_afButtonReleased & IN_AIM)
            {
                Weapon_AimToggle(pWep);
            }
        }

        previouslyReloading = pWep->m_bInReload;
    }

    if (m_afButtonPressed & IN_DROP)
    {
        Vector eyeForward;
        EyeVectors(&eyeForward);
        const float forwardOffset = 250.0f;
        eyeForward *= forwardOffset;

        Weapon_Drop(GetActiveWeapon(), NULL, &eyeForward);
    }

#if(0)
    SetSimulationTime(gpGlobals->curtime);
    SetNextThink(gpGlobals->curtime + 0.01f);
#endif
}

void CNEO_Player::PlayerDeathThink()
{
    BaseClass::PlayerDeathThink();
}

void CNEO_Player::Weapon_AimToggle( CBaseCombatWeapon *pWep )
{
    if (!IsAllowedToZoom(pWep))
    {
        return;
    }

    bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR);
    Weapon_SetZoom(showCrosshair);
}

inline void CNEO_Player::Weapon_SetZoom(bool bZoomIn)
{
    const float zoomSpeedSecs = 0.25f;

    ShowCrosshair(bZoomIn);
    
    if (bZoomIn)
    {
        const int zoomAmount = 30;
        SetFOV((CBaseEntity*)this, GetDefaultFOV() - zoomAmount, zoomSpeedSecs);
    }
    else
    {
        SetFOV((CBaseEntity*)this, GetDefaultFOV(), zoomSpeedSecs);
    }
}

void CNEO_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
    int animDesired;

	float speed;

	speed = GetAbsVelocity().Length2D();

    if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

    Activity idealActivity = ACT_IDLE;

    if ( playerAnim == PLAYER_JUMP )
    {
        idealActivity = ACT_HOP;
    }
    else if ( playerAnim == PLAYER_DIE )
    {
        if ( m_lifeState == LIFE_ALIVE )
        {
            return;
        }
    }
    else if ( playerAnim == PLAYER_ATTACK1 )
    {
        if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
        else
        {
            idealActivity = ACT_RANGE_ATTACK1;
        }
    }
    else if ( playerAnim == PLAYER_RELOAD )
    {
        idealActivity = ACT_RELOAD;
    }
    else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
    {
        // Still jumping
        if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HOP )
        {
            idealActivity = GetActivity();
        }
        else
        {
            if ( GetFlags() & FL_DUCKING )
            {
                if ( speed > 0 )
				{
					idealActivity = ACT_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_CROUCHIDLE;
				}
            }
            else
			{
				if ( speed > 0 )
				{
					{
						idealActivity = ACT_RUN;
					}
				}
				else
				{
					idealActivity = ACT_IDLE;
				}
			}
        }
    }

    SetActivity(idealActivity);

    animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

    if (animDesired == -1)
    {
        animDesired = SelectWeightedSequence( idealActivity );

        if ( animDesired == -1 )
        {
            animDesired = 0;
        }
    }

    // Already using the desired animation?
    if ( GetSequence() == animDesired )
        return;
    
    m_flPlaybackRate = 1.0;
    ResetSequence( animDesired );
    SetCycle( 0 );
}

bool CNEO_Player::HandleCommand_JoinTeam( int team )
{
    SetPlayerTeamModel();

    return BaseClass::HandleCommand_JoinTeam(team);
}

bool CNEO_Player::ClientCommand( const CCommand &args )
{
    return BaseClass::ClientCommand(args);
}

void CNEO_Player::CreateViewModel( int index )
{
    Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
    {
        return;
    }

	CNEOPredictedViewModel *vm = ( CNEOPredictedViewModel * )CreateEntityByName( "neo_predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
    else
    {
        Warning("CNEO_Player::CreateViewModel: Failed to create neo_predicted_viewmodel\n");
        return;
    }
}

bool CNEO_Player::BecomeRagdollOnClient( const Vector &force )
{
    return BaseClass::BecomeRagdollOnClient(force);
}

void CNEO_Player::Event_Killed( const CTakeDamageInfo &info )
{
    BaseClass::Event_Killed(info);
}

int CNEO_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
    return BaseClass::OnTakeDamage(inputInfo);
}

bool CNEO_Player::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer,
    const CUserCmd *pCmd,
    const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
    return BaseClass::WantsLagCompensationOnEntity(pPlayer, pCmd,
        pEntityTransmitBits);
}

void CNEO_Player::FireBullets ( const FireBulletsInfo_t &info )
{
    BaseClass::FireBullets(info);
}

bool CNEO_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon,
    int viewmodelindex )
{
    ShowCrosshair(false);

    return BaseClass::Weapon_Switch(pWeapon, viewmodelindex);
}

bool CNEO_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
    return BaseClass::BumpWeapon(pWeapon);
}

void CNEO_Player::ChangeTeam( int iTeam )
{
    BaseClass::ChangeTeam(iTeam);
}

void CNEO_Player::SetPlayerTeamModel( void )
{
    CNEOModelManager *modelManager = CNEOModelManager::Instance();
    if (!modelManager)
    {
        Assert(false);
        return;
    }

    // Pick a random skin from random class.
    // NEO TODO (Rain): pick the appropriate one
    // based on player's class and skin selection.
    const char *model = modelManager->GetPlayerModel(
        (NeoSkin)RandomInt(0, 2),
        (NeoClass)RandomInt(0, NEO_CLASS_ENUM_COUNT - 1),
        GetTeamNumber());

    if (!*model)
    {
        Assert(false);
        return;
    }

    SetModel(model);
    //DevMsg("Set model: %s\n", model);
    //SetupPlayerSoundsByModel(model); // TODO
}

void CNEO_Player::PickupObject( CBaseEntity *pObject,
    bool bLimitMassAndSize )
{
    BaseClass::PickupObject(pObject, bLimitMassAndSize);
}

void CNEO_Player::PlayStepSound( Vector &vecOrigin,
    surfacedata_t *psurface, float fvol, bool force )
{
    BaseClass::PlayStepSound(vecOrigin, psurface, fvol, force);
}

// Is the player allowed to drop a weapon of this type?
// NEO TODO (Rain): Forbid Neo drops like knife, neo nades etc.
// once they have been implemented.
inline bool CNEO_Player::IsAllowedToDrop(CBaseCombatWeapon *pWep)
{
    if (!pWep)
    {
        return false;
    }

    const char *unallowedDrops[] = {
        "weapon_frag",
    };

    CBaseCombatWeapon *pTest = NULL;
    for (int i = 0; i < ARRAYSIZE(unallowedDrops); i++)
    {
        pTest = Weapon_OwnsThisType(unallowedDrops[i]);
        if (pWep == pTest)
        {
            return false;
        }
    }

    return true;
}

// Is the player allowed to aim zoom with a weapon of this type?
inline bool CNEO_Player::IsAllowedToZoom(CBaseCombatWeapon *pWep)
{
    if (!pWep)
    {
        return false;
    }

    const char *allowedAimZoom[] = {
        "weapon_tachi",
    };

    CBaseCombatWeapon *pTest = NULL;
    for (int i = 0; i < ARRAYSIZE(allowedAimZoom); i++)
    {
        pTest = Weapon_OwnsThisType(allowedAimZoom[i]);
        if (pWep == pTest)
        {
            return true;
        }
    }

    return false;
}

void CNEO_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon,
    const Vector *pvecTarget, const Vector *pVelocity )
{
    if (!IsDead() && pWeapon)
    {
        if (!IsAllowedToDrop(pWeapon))
        {
            // NEO TODO (Rain): No need for server to tell
            // client to play a sound effect, move this to client.
            {
#if (0)
                CRecipientFilter filter;
                filter.AddRecipient(this);

                EmitSound_t sound;
                sound.m_nChannel = CHAN_ITEM;
                // NEO TODO (Rain): Find appropriate sound for denying drop.
                // Possibly the original NT "can't +use" sound.
                sound.m_pSoundName = "player/CPneutral.wav";
                sound.m_SoundLevel = SNDLVL_30dB;
                sound.m_flVolume = VOL_NORM;

                // NEO HACK/FIXME (Rain): This should get early precached
                // as part of our regular sound scripts on this::Precache()
                PrecacheScriptSound(sound.m_pSoundName);

                EmitSound(filter, edict()->m_EdictIndex, sound);
#endif
            }

            return;
        }
    }

    BaseClass::Weapon_Drop(pWeapon, pvecTarget, pVelocity);
}

void CNEO_Player::UpdateOnRemove( void )
{
    BaseClass::UpdateOnRemove();
}

void CNEO_Player::DeathSound( const CTakeDamageInfo &info )
{
    BaseClass::DeathSound(info);
}

CBaseEntity* CNEO_Player::EntSelectSpawnPoint( void )
{
    return BaseClass::EntSelectSpawnPoint();
}

bool CNEO_Player::StartObserverMode(int mode)
{
    return BaseClass::StartObserverMode(mode);
}

void CNEO_Player::StopObserverMode()
{
    BaseClass::StopObserverMode();
}

bool CNEO_Player::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
    return BaseClass::CanHearAndReadChatFrom(pPlayer);
}