//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "sdk_basegrenade_projectile.h"

float GetCurrentGravity( void );


#ifdef CLIENT_DLL

#ifndef NEO
	#include "c_sdk_player.h"
#else
#include "neo_gamerules.h"
#endif // NEO
#else

	#include "soundent.h"

	BEGIN_DATADESC( CBaseGrenadeProjectile )
		DEFINE_THINKFUNC( DangerSoundThink ),
	END_DATADESC()

#endif


IMPLEMENT_NETWORKCLASS_ALIASED( BaseGrenadeProjectile, DT_BaseGrenadeProjectile )

BEGIN_NETWORK_TABLE( CBaseGrenadeProjectile, DT_BaseGrenadeProjectile )
	#ifdef CLIENT_DLL
		RecvPropVector( RECVINFO( m_vInitialVelocity ) )
	#else
		SendPropVector( SENDINFO( m_vInitialVelocity ), 
			20,		// nbits
			0,		// flags
			-3000,	// low value
			3000	// high value
			)
	#endif
END_NETWORK_TABLE()


#ifdef NEO
ConVar sv_neo_grenade_show_path("sv_neo_grenade_show_path", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Whether to show a grenade's path to all players, and for how long", true, 0, true, 15.f);
#endif // NEO
#ifdef CLIENT_DLL
#ifdef NEO
// We're not clearing the debug overlay atm, make sure this is very short
ConVar cl_neo_grenade_show_path("cl_neo_grenade_show_path", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Whether to show a grenade's path when spectating, and for how long", true, 0, true, 2.f);
#endif // NEO


	void CBaseGrenadeProjectile::PostDataUpdate( DataUpdateType_t type )
	{
		BaseClass::PostDataUpdate( type );

		if ( type == DATA_UPDATE_CREATED )
		{
			// Now stick our initial velocity into the interpolation history 
			CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();
			
			interpolator.ClearHistory();
			float changeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

			// Add a sample 1 second back.
			Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
			interpolator.AddToHead( changeTime - 1.0, &vCurOrigin, false );

			// Add the current sample.
			vCurOrigin = GetLocalOrigin();
			interpolator.AddToHead( changeTime, &vCurOrigin, false );
		}
	}

#ifdef NEO
#include "c_neo_player.h"
#ifdef GLOWS_ENABLE
	extern ConVar glow_outline_effect_enable;
#endif // GLOWS_ENABLE
#endif // NEO
	int CBaseGrenadeProjectile::DrawModel( int flags )
	{
#ifndef NEO
		// During the first half-second of our life, don't draw ourselves if he's
		// still playing his throw animation.
		// (better yet, we could draw ourselves in his hand).
		if ( GetThrower() != C_BasePlayer::GetLocalPlayer() )
		{
			if ( gpGlobals->curtime - m_flSpawnTime < 0.5 )
			{
				C_SDKPlayer *pPlayer = dynamic_cast<C_SDKPlayer*>( GetThrower() );
				if ( pPlayer && pPlayer->m_PlayerAnimState->IsThrowingGrenade() )
				{
					return 0;
				}
			}
		}
#endif // NEO
#ifdef NEO
		auto pTargetPlayer = C_NEO_Player::GetVisionTargetNEOPlayer();
		bool inThermalVision = pTargetPlayer->IsInVision() && pTargetPlayer->GetClass() == NEO_CLASS_SUPPORT;
		if (inThermalVision)
		{
			int ret = 0;
			IMaterial* pass = materials->FindMaterial("dev/thermal_grenade_projectile_model", TEXTURE_GROUP_MODEL);
			modelrender->ForcedMaterialOverride(pass);
			ret |= BaseClass::DrawModel(flags);
			modelrender->ForcedMaterialOverride(nullptr);
			return ret;
		}
#endif // NEO
		return BaseClass::DrawModel( flags );
	}

	void CBaseGrenadeProjectile::Spawn()
	{
		m_flSpawnTime = gpGlobals->curtime;
		BaseClass::Spawn();
#ifdef NEO
		m_flTemperature = THERMALS_OBJECT_TEMPERATURE_HELD;	// NEO NOTE (Adam) The server doesn't know the client side temperature of the weapon that spawned this projectile, and the client doesn't know what weapon spawned this projectile (may not exist already)
															// NEO TODO (Adam) use the temperature of the weapon at the time this projectile was created as the starting projectile temperature.
		SetNextClientThink(gpGlobals->curtime + TICK_INTERVAL);
#endif // NEO
	}

#ifdef NEO

	void CBaseGrenadeProjectile::ClientThink()
	{
		m_flTemperature = Max(THERMALS_OBJECT_MIN_TEMPERATURE, m_flTemperature - (TICK_INTERVAL * THERMALS_OBJECT_COOL_RATE));
		if (m_flTemperature > THERMALS_OBJECT_MIN_TEMPERATURE)
		{
			SetNextClientThink(gpGlobals->curtime + TICK_INTERVAL);
		}
		if (cl_neo_grenade_show_path.GetBool() || sv_neo_grenade_show_path.GetBool())
		{
			DrawPath();
			SetNextClientThink(gpGlobals->curtime + TICK_INTERVAL);
		}
	}
	
	void CBaseGrenadeProjectile::DrawPath()
	{
		CBasePlayer *player = UTIL_PlayerByIndex(GetLocalPlayerIndex());
		if ( player == NULL )
			return;

		const bool showPathInSpec = cl_neo_grenade_show_path.GetBool() && player->GetTeamNumber() == TEAM_SPECTATOR;
		const bool showPathWhenServerEnabled = sv_neo_grenade_show_path.GetBool();
		if (!showPathInSpec && !showPathWhenServerEnabled)
			return;

		const Vector origin = GetAbsOrigin();
		if (!m_vLastDrawPosition.IsValid())
		{
			m_vLastDrawPosition = origin;
			return;
		}

		if (m_vLastDrawPosition.DistToSqr(origin) < 0.1f)
			return;

		float r, g, b = 0;
		NEORules()->GetTeamGlowColor(GetTeamNumber(), r, g, b);
		r *= 255;
		g *= 255;
		b *= 255;
		if (GetDamage())
		{
			const float timeAlive = (gpGlobals->curtime - m_flNeoCreateTime) * 0.5f;
			r = Min(255.f, Max(0.f, r * (1 - timeAlive) + (255 * timeAlive)));
			g = Max(0.f, g * (1 - timeAlive));
			b = Max(0.f, b * (1 - timeAlive));
		}
		DebugDrawLine(m_vLastDrawPosition, origin, r, g, b, true, showPathWhenServerEnabled ? sv_neo_grenade_show_path.GetFloat() : cl_neo_grenade_show_path.GetFloat());
		m_vLastDrawPosition = origin;
	}
#endif // NEO

#else

	void CBaseGrenadeProjectile::Spawn( void )
	{
		BaseClass::Spawn();

		SetSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		SetSolid( SOLID_BBOX );	// So it will collide with physics props!

		// smaller, cube bounding box so we rest on the ground
		SetSize( Vector ( -2, -2, -2 ), Vector ( 2, 2, 2 ) );

		if (sv_neo_grenade_show_path.GetBool())
		{ // Transmit the grenade to all players so they can see the path outside their pvs
			SetTransmitState(FL_EDICT_ALWAYS);
		}
		else
		{ // Spectators will still want to see the grenade if they have cl_neo_grenade_show_path set
			SetTransmitState(FL_EDICT_FULLCHECK);
		}
	}

	void CBaseGrenadeProjectile::DangerSoundThink( void )
	{
		if (!IsInWorld())
		{
			Remove( );
			return;
		}

		if( gpGlobals->curtime > m_flDetonateTime )
		{
			Detonate();
			return;
		}

		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, GetAbsVelocity().Length( ), 0.2 );

		SetNextThink( gpGlobals->curtime + 0.2 );

		if (GetWaterLevel() != 0)
		{
			SetAbsVelocity( GetAbsVelocity() * 0.5 );
		}
	}

	//Sets the time at which the grenade will explode
	void CBaseGrenadeProjectile::SetDetonateTimerLength( float timer )
	{
		m_flDetonateTime = gpGlobals->curtime + timer;
	}

	void CBaseGrenadeProjectile::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
	{
		//Assume all surfaces have the same elasticity
		float flSurfaceElasticity = 1.0;

		//Don't bounce off of players with perfect elasticity
		if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
		{
			flSurfaceElasticity = 0.3;
		}

		// if its breakable glass and we kill it, don't bounce.
		// give some damage to the glass, and if it breaks, pass 
		// through it.
		bool breakthrough = false;

		if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable" ) )
		{
			breakthrough = true;
		}

		if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable_surf" ) )
		{
			breakthrough = true;
		}

		if (breakthrough)
		{
			CTakeDamageInfo info( this, this, 10, DMG_CLUB );
			trace.m_pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

			ApplyMultiDamage();

			if( trace.m_pEnt->m_iHealth <= 0 )
			{
				// slow our flight a little bit
				Vector vel = GetAbsVelocity();

				vel *= 0.4;

				SetAbsVelocity( vel );
				return;
			}
		}
		
		float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
		flTotalElasticity = clamp( flTotalElasticity, 0.0f, 0.9f );

		// NOTE: A backoff of 2.0f is a reflection
		Vector vecAbsVelocity;
		PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
		vecAbsVelocity *= flTotalElasticity;

		// Get the total velocity (player + conveyors, etc.)
		VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
		float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

		// Stop if on ground.
		if ( trace.plane.normal.z > 0.7f )			// Floor
		{
			// Verify that we have an entity.
			CBaseEntity *pEntity = trace.m_pEnt;
			Assert( pEntity );

			SetAbsVelocity( vecAbsVelocity );

			if ( flSpeedSqr < ( 30 * 30 ) )
			{
				if ( pEntity->IsStandable() )
				{
					SetGroundEntity( pEntity );
				}

				// Reset velocities.
				SetAbsVelocity( vec3_origin );
				SetLocalAngularVelocity( vec3_angle );

				//align to the ground so we're not standing on end
				QAngle angle;
				VectorAngles( trace.plane.normal, angle );

				// rotate randomly in yaw
				angle[1] = random->RandomFloat( 0, 360 );

				// TODO: rotate around trace.plane.normal
				
				SetAbsAngles( angle );			
			}
			else
			{
				Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;	
				Vector vecBaseDir = GetBaseVelocity();
				VectorNormalize( vecBaseDir );
				float flScale = vecDelta.Dot( vecBaseDir );

				VectorScale( vecAbsVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, vecVelocity ); 
				VectorMA( vecVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
				PhysicsPushEntity( vecVelocity, &trace );
			}
		}
		else
		{
			// If we get *too* slow, we'll stick without ever coming to rest because
			// we'll get pushed down by gravity faster than we can escape from the wall.
			if ( flSpeedSqr < ( 30 * 30 ) )
			{
				// Reset velocities.
				SetAbsVelocity( vec3_origin );
				SetLocalAngularVelocity( vec3_angle );
			}
			else
			{
				SetAbsVelocity( vecAbsVelocity );
			}
		}
		
		BounceSound();
	}

	void CBaseGrenadeProjectile::SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )
	{
		m_vInitialVelocity = velocity;
	}

#endif
