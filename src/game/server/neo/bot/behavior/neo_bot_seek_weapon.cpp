#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/neo_bot_path_compute.h"
#include "weapon_neobasecombatweapon.h"
#include "neo_player_shared.h"
#include "nav_mesh.h"

//---------------------------------------------------------------------------------------------
// Mask of non-identity "tag" bits that GetNeoWepBits() may include
static constexpr NEO_WEP_BITS_UNDERLYING_TYPE WEP_TAG_BITS_MASK =
	NEO_WEP_SCOPEDWEAPON | NEO_WEP_THROWABLE | NEO_WEP_SUPPRESSED | NEO_WEP_FIREARM | NEO_WEP_EXPLOSIVE;

static constexpr int BOT_WEP_PREF_RANK_UNPREFERRED = -1;
static constexpr int BOT_WEP_PREF_RANK_EMPTY = -2;

//---------------------------------------------------------------------------------------------
bool IsUndroppablePrimary( CBaseCombatWeapon *pPrimary )
{
	if ( !pPrimary )
	{
		return false;
	}

	CNEOBaseCombatWeapon *pNeoWep = ToNEOWeapon( pPrimary );
	if ( !pNeoWep )
	{
		return false;
	}

	const NEO_WEP_BITS_UNDERLYING_TYPE wepBits = pNeoWep->GetNeoWepBits();
	return ( wepBits & NEO_WEP_BALC ) != 0;
}

//---------------------------------------------------------------------------------------------
int GetBotWeaponPreferenceRank( CNEOBot *me, NEO_WEP_BITS_UNDERLYING_TYPE wepBit )
{
	// Strip tag bits to get the primary identity bit
	const NEO_WEP_BITS_UNDERLYING_TYPE identityBit = wepBit & ~WEP_TAG_BITS_MASK;

	const int playerClass = me->GetClass();
	if ( playerClass < 0 || playerClass >= NEO_CLASS__LOADOUTABLE_COUNT )
	{
		return BOT_WEP_PREF_RANK_UNPREFERRED;
	}

	for ( int idxRank = NEO_RANK__TOTAL - 1; idxRank >= 0; --idxRank )
	{
		if ( me->m_profile.flagsWepPrefs[playerClass][idxRank] & identityBit )
		{
			return idxRank;
		}
	}

	return BOT_WEP_PREF_RANK_UNPREFERRED;
}

//---------------------------------------------------------------------------------------------
bool IsWeaponPreferenceUpgrade( CNEOBot *me, CNEOBaseCombatWeapon *pTargetWep, int myPrefRank, bool bHasReserveAmmo )
{
	if ( !pTargetWep )
	{
		return false;
	}

	if ( myPrefRank == BOT_WEP_PREF_RANK_EMPTY )
	{
		// We have no primary weapon at all, anything is an upgrade
		return true;
	}

	if ( !bHasReserveAmmo )
	{
		if ( pTargetWep->GetPrimaryAmmoCount() > 0 )
		{
			return true;
		}
	}

	const NEO_WEP_BITS_UNDERLYING_TYPE targetWepBits = pTargetWep->GetNeoWepBits();
	const int targetPrefRank = GetBotWeaponPreferenceRank( me, targetWepBits );
	
	if ( targetPrefRank <= BOT_WEP_PREF_RANK_UNPREFERRED && bHasReserveAmmo )
	{
		return false;
	}

	if ( myPrefRank >= BOT_WEP_PREF_RANK_UNPREFERRED && targetPrefRank <= myPrefRank )
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------
CBaseEntity *FindNearestPrimaryWeapon( CNEOBot *me, bool bAllowDropGhost, CNEOIgnoredWeaponsCache *pIgnoredWeapons )
{
	constexpr float flSearchRadius = 1000.0f;
	CBaseEntity *pClosestWeapon = nullptr;
	float flClosestDistSq = FLT_MAX;
	int iBestWeaponRank = -999;

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( pPrimary )
	{
		CNEOBaseCombatWeapon *pNeoPrimary = ToNEOWeapon( pPrimary );
		if ( pNeoPrimary && ( pNeoPrimary->GetNeoWepBits() & NEO_WEP_BALC ) )
		{
			// can't switch these weapons
			return nullptr;
		}

		if ( !bAllowDropGhost )
		{
			if ( pNeoPrimary->GetNeoWepBits() & NEO_WEP_GHOST )
			{
				// Hold onto the ghost unless we are allowed to by the situation
				return nullptr;
			}
			
			if ( me->GetVisionInterface()->GetPrimaryKnownThreat( true ) != nullptr )
			{
				// Too exposed to drop current weapon
				return nullptr;
			}
		}
	}

	int myPrefRank = BOT_WEP_PREF_RANK_EMPTY;
	bool bHasReserveAmmo = false;
	if ( pPrimary )
	{
		myPrefRank = BOT_WEP_PREF_RANK_UNPREFERRED;
		bHasReserveAmmo = pPrimary->GetPrimaryAmmoCount() > 0;
		CNEOBaseCombatWeapon *pMyNeoWep = ToNEOWeapon( pPrimary );
		if ( pMyNeoWep )
		{
			myPrefRank = GetBotWeaponPreferenceRank( me, pMyNeoWep->GetNeoWepBits() );
		}
	}

	// For checking if weapon candidate is in PVS of me
	CNavArea *pMyArea = me->GetLastKnownArea();
	if ( !pMyArea )
	{
		pMyArea = TheNavMesh->GetNearestNavArea( me->GetAbsOrigin() );
	}

	// Iterate through all available weapons, looking for the nearest primary 
	CBaseEntity *pEntity = nullptr;
	while ( ( pEntity = gEntList.FindEntityByClassnameWithin( pEntity, "weapon_*", me->GetAbsOrigin(), flSearchRadius ) ) )
	{
		if ( pIgnoredWeapons && pIgnoredWeapons->Has( pEntity ) )
		{
			continue;
		}

		CBaseCombatWeapon *pWeapon = pEntity->MyCombatWeaponPointer();
		if ( pWeapon && !pWeapon->GetOwner() && pWeapon->HasAnyAmmo() && pWeapon->GetSlot() == 0 )
		{
			CNEOBaseCombatWeapon *pNeoWeapon = ToNEOWeapon( pWeapon );
			if ( !pNeoWeapon || ( pNeoWeapon->GetNeoWepBits() & NEO_WEP_GHOST ) )
			{
				continue;
			}

			// only consider weapons in the bot's wishlist that are an upgrade
			if ( !IsWeaponPreferenceUpgrade( me, pNeoWeapon, myPrefRank, bHasReserveAmmo ) )
			{
				continue;
			}
			
			const int targetPrefRank = GetBotWeaponPreferenceRank( me, pNeoWeapon->GetNeoWepBits() );
			
			float flDistSq = me->GetAbsOrigin().DistToSqr( pEntity->GetAbsOrigin() );
			
			bool bBetterFound = false;
			if ( targetPrefRank > iBestWeaponRank )
			{
				bBetterFound = true;
			}
			else if ( targetPrefRank == iBestWeaponRank && flDistSq < flClosestDistSq )
			{
				bBetterFound = true;
			}

			if ( bBetterFound )
			{
				// Check if weapon candidate is in PVS of me
				if ( pMyArea )
				{
					CNavArea *pWepArea = TheNavMesh->GetNavArea( pEntity->WorldSpaceCenter() );
					if ( pWepArea && !pMyArea->IsPotentiallyVisible( pWepArea ) )
					{
						continue;
					}
				}

				flClosestDistSq = flDistSq;
				iBestWeaponRank = targetPrefRank;
				pClosestWeapon = pEntity;
			}
		}
	}

	return pClosestWeapon;
}

//---------------------------------------------------------------------------------------------
CNEOBotSeekWeapon::CNEOBotSeekWeapon( CBaseEntity *pTargetWeapon, CNEOIgnoredWeaponsCache *pIgnoredWeapons )
{
	m_hTargetWeapon = pTargetWeapon;
	m_pIgnoredWeapons = pIgnoredWeapons;
}

//---------------------------------------------------------------------------------------------
CBaseEntity *CNEOBotSeekWeapon::FindAndPathToWeapon( CNEOBot *me )
{
	if ( !m_hTargetWeapon )
	{
		m_hTargetWeapon = FindNearestPrimaryWeapon( me, false, m_pIgnoredWeapons );
	}
	
	if ( m_hTargetWeapon )
	{
		if ( !CNEOBotPathCompute( me, m_path, m_hTargetWeapon->GetAbsOrigin(), FASTEST_ROUTE ) || !m_path.IsValid() )
		{
			m_hTargetWeapon = nullptr;
			m_path.Invalidate();
		}
	}
	else
	{
		// no weapon found
		m_path.Invalidate();
	}

	return m_hTargetWeapon;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotSeekWeapon::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_repathTimer.Invalidate();
	m_giveUpTimer.Start( 10.0f );
	m_path.Invalidate();

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( IsUndroppablePrimary( pPrimary ) )
	{
		// Don't scavenge if we have a weapon like the balc
		return Done("Equipped with an un-droppable weapon, will not seek");
	}

	if ( !m_hTargetWeapon )
	{
		m_hTargetWeapon = FindNearestPrimaryWeapon( me, false, m_pIgnoredWeapons );
	}

	if ( !m_hTargetWeapon )
	{
		return Done("No valid replacement primary found");
	}

	CNEOBaseCombatWeapon *pNeoPrimary = ToNEOWeapon( pPrimary );
	if ( pNeoPrimary && ( pNeoPrimary->GetNeoWepBits() & NEO_WEP_GHOST ) )
	{
		// Try to drop once, but sometimes the environment causes a bounce back
		me->DropPrimaryWeapon();
	}

	// found a replacement, go towards it next frame
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotSeekWeapon::Update( CNEOBot *me, float interval )
{
	if ( !m_hTargetWeapon )
	{
		return Done("No weapon to seek");
	}

	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done("Gave up seeking weapon");
	}

	if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
	{
		if ( !CNEOBotPathCompute( me, m_path, m_hTargetWeapon->GetAbsOrigin(), FASTEST_ROUTE ) )
		{
			return Done("Unable to find a path to the nearest primary weapon");
		}
		m_repathTimer.Start( RandomFloat( 0.3f, 0.6f ) );
	}

	if (!m_path.IsValid())
	{
		return Done("Path to weapon is invalid");
	}
	
	// Verify the path actually reaches the weapon
	const float flMaxEndpointDistSqr = 150.0f * 150.0f;
	if ( m_path.GetEndPosition().DistToSqr( m_hTargetWeapon->GetAbsOrigin() ) > flMaxEndpointDistSqr )
	{
		if ( m_pIgnoredWeapons && !m_pIgnoredWeapons->Has( m_hTargetWeapon ) )
		{
			m_pIgnoredWeapons->Add( m_hTargetWeapon );
		}
		return Done("Weapon is unreachable (path doesn't get close enough)");
	}

	m_path.Update(me);

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot(0);
	if ( pPrimary )
	{
		int myPrefRank = BOT_WEP_PREF_RANK_UNPREFERRED;
		CNEOBaseCombatWeapon *pMyNeoWep = ToNEOWeapon( pPrimary );
		if ( pMyNeoWep )
		{
			myPrefRank = GetBotWeaponPreferenceRank( me, pMyNeoWep->GetNeoWepBits() );
		}

		CNEOBaseCombatWeapon *pTargetNeoWep = ToNEOWeapon( m_hTargetWeapon.Get() );
		if ( !pTargetNeoWep )
		{
			return Done( "Target weapon is invalid or not a NEO weapon" );
		}

		const bool bIsUpgrade = IsWeaponPreferenceUpgrade( me, pTargetNeoWep, myPrefRank, pPrimary->GetPrimaryAmmoCount() > 0 );

		if ( bIsUpgrade )
		{
			// We are seeking a better weapon, check if we are close enough to drop ours
			const float flDropDistSqr = 100.0f * 100.0f;
			if ( me->GetAbsOrigin().DistToSqr( m_hTargetWeapon->GetAbsOrigin() ) <= flDropDistSqr || 
				( pMyNeoWep && ( pMyNeoWep->GetNeoWepBits() & NEO_WEP_GHOST ) ) )
			{
				me->DropPrimaryWeapon();
			}
		}
		else if ( pPrimary->HasAnyAmmo() ) // Ensure it's not empty
		{
			// We already have an equal or better weapon, or we somehow picked up another weapon
			// so we don't exit this behavior still equipped with secondary
			me->Weapon_Switch(pPrimary);
			return Done("Acquired a primary weapon");
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotSeekWeapon::OnResume( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_hTargetWeapon = nullptr;
	m_repathTimer.Invalidate();
	if ( !FindAndPathToWeapon( me ) )
	{
		return Done( "No weapon found on resume" );
	}
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekWeapon::OnStuck( CNEOBot *me )
{
	m_hTargetWeapon = nullptr;
	m_repathTimer.Invalidate();
	FindAndPathToWeapon(me);
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekWeapon::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryDone();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekWeapon::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_hTargetWeapon = nullptr;
	m_repathTimer.Invalidate();
	FindAndPathToWeapon(me);
	return TryContinue();
}
