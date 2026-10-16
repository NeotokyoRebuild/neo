#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_player_shared.h"
#include "nav_mesh.h"

//---------------------------------------------------------------------------------------------
static constexpr int BOT_WEP_PREF_RANK_UNPREFERRED = -1;
static constexpr int BOT_WEP_PREF_RANK_EMPTY = -2;

// A weapon lies on a nav area when it is over the area, or at most half a player's width past its edge
// (nav_generate areas often stop short of wall boundaries),
static constexpr float BOT_WEP_MAX_OUTSIDE_AREA = HalfHumanWidth;
// and at most this far above the area's surface (such as a desk or crate surface)
static constexpr float BOT_WEP_MAX_HEIGHT_ABOVE_AREA = 32.0f;
// or this far below it (a nav area's plane is rarely an exact flush fit with a stairway's steps)
static constexpr float BOT_WEP_MAX_DEPTH_BELOW_AREA = 8.0f;
static constexpr float BOT_WEP_AREA_SEARCH_RANGE = 64.0f;

//---------------------------------------------------------------------------------------------
// Weapons can come to rest where no nav area exists: in a pit, behind a counter, out of a window
static CNavArea *GetWeaponNavArea( CBaseEntity *pWeapon )
{
	const Vector vecWeapon = pWeapon->WorldSpaceCenter();
	CNavArea *pArea = TheNavMesh->GetNearestNavArea( vecWeapon, false, BOT_WEP_AREA_SEARCH_RANGE, true, false );
	if ( !pArea )
	{
		return nullptr;
	}

	Vector vecClosest;
	pArea->GetClosestPointOnArea( vecWeapon, &vecClosest );
	if ( ( vecWeapon - vecClosest ).Length2D() > BOT_WEP_MAX_OUTSIDE_AREA )
	{
		return nullptr;
	}

	const float flHeightAboveArea = vecWeapon.z - vecClosest.z;
	if ( flHeightAboveArea > BOT_WEP_MAX_HEIGHT_ABOVE_AREA || flHeightAboveArea < -BOT_WEP_MAX_DEPTH_BELOW_AREA )
	{
		return nullptr;
	}

	return pArea;
}

//---------------------------------------------------------------------------------------------
bool IsUndroppablePrimary( CBaseCombatWeapon *pPrimary )
{
	if ( !pPrimary )
	{
		return false;
	}

	auto *pNeoWep = assert_cast<CNEOBaseCombatWeapon *>( pPrimary );
	return !pNeoWep->CanDrop();
}

//---------------------------------------------------------------------------------------------
int GetBotWeaponPreferenceRank( const CNEOBot *me, NEO_WEP_BITS_UNDERLYING_TYPE wepBit )
{
	const int playerClass = me->GetClass();
	if ( playerClass < 0 || playerClass >= NEO_CLASS__LOADOUTABLE_COUNT )
	{
		return BOT_WEP_PREF_RANK_UNPREFERRED;
	}

	for ( int idxRank = NEO_RANK__TOTAL - 1; idxRank >= 0; --idxRank )
	{
		if ( me->m_profile.flagsLootWepPrefs[playerClass][idxRank] & wepBit )
		{
			return idxRank;
		}
	}

	return BOT_WEP_PREF_RANK_UNPREFERRED;
}

//---------------------------------------------------------------------------------------------
bool IsWeaponPreferenceUpgrade( const CNEOBot *me, CNEOBaseCombatWeapon *pTargetWep, int myPrefRank, bool bHasReserveAmmo )
{
	if ( !pTargetWep )
	{
		return false;
	}

	if ( pTargetWep->GetPrimaryAmmoCount() <= 0 )
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
		return true;
	}

	const auto targetWepBits = pTargetWep->GetNeoWepBits();
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
CBaseEntity *FindNearestPrimaryWeapon( const CNEOBot *me, bool bAllowDropGhost, const CNEOIgnoredWeaponsCache *pIgnoredWeapons )
{
	constexpr float flSearchRadius = 1000.0f;
	CBaseEntity *pClosestWeapon = nullptr;
	float flClosestDistSq = FLT_MAX;
	int iBestWeaponRank = -999;

	int myPrefRank = BOT_WEP_PREF_RANK_EMPTY;
	bool bHasReserveAmmo = false;

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( pPrimary )
	{
		auto *pNeoPrimary = assert_cast<CNEOBaseCombatWeapon *>( pPrimary );
		if ( !pNeoPrimary->CanDrop() )
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

		bHasReserveAmmo = pPrimary->GetPrimaryAmmoCount() > 0;
		myPrefRank = GetBotWeaponPreferenceRank( me, pNeoPrimary->GetNeoWepBits() );
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
			auto *pNeoWeapon = assert_cast<CNEOBaseCombatWeapon *>( pWeapon );
			if ( pNeoWeapon->GetNeoWepBits() & NEO_WEP_GHOST )
			{
				continue;
			}

			if ( !pNeoWeapon->CanBePickedUpByClass( me->GetClass() ) )
			{
				continue;
			}

			// only consider weapons in the bot's wishlist that are an upgrade
			if ( !IsWeaponPreferenceUpgrade( me, pNeoWeapon, myPrefRank, bHasReserveAmmo ) )
			{
				continue;
			}
			
			const auto targetWepBits = pNeoWeapon->GetNeoWepBits();
			const int targetPrefRank = GetBotWeaponPreferenceRank( me, targetWepBits );
			
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
				// A path to an off-mesh weapon ends with a straight walk to it, over whatever lies between
				CNavArea *pWepArea = GetWeaponNavArea( pEntity );
				if ( !pWepArea )
				{
					continue;
				}

				// Check if weapon candidate is in PVS of me
				if ( pMyArea && !pMyArea->IsPotentiallyVisible( pWepArea ) )
				{
					continue;
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
// Remember a weapon the bot failed to reach, so the next scavenge does not pick it again
void CNEOBotSeekWeapon::IgnoreTargetWeapon( void )
{
	if ( !m_hTargetWeapon || !m_pIgnoredWeapons || m_pIgnoredWeapons->Has( m_hTargetWeapon ) )
	{
		return;
	}

	m_pIgnoredWeapons->Add( m_hTargetWeapon );
}

//---------------------------------------------------------------------------------------------
// Only a full path to the weapon's nav area: a partial one ends with a straight walk from the edge of the mesh
bool CNEOBotSeekWeapon::PathToTargetWeapon( CNEOBot *me )
{
	// The weapon may have been knocked off the mesh since it was chosen
	const CNavArea *pWepArea = GetWeaponNavArea( m_hTargetWeapon );
	if ( !pWepArea )
	{
		m_path.Invalidate();
		return false;
	}

	// Stay on the area: from its edge a bot still touches a weapon lying against the wall beyond it
	Vector vecGoal;
	pWepArea->GetClosestPointOnArea( m_hTargetWeapon->WorldSpaceCenter(), &vecGoal );
	return CNEOBotPathCompute( me, m_path, vecGoal, FASTEST_ROUTE, PATH_NO_LENGTH_LIMIT, PATH_TRUNCATE_INCOMPLETE_PATH );
}

//---------------------------------------------------------------------------------------------
CBaseEntity *CNEOBotSeekWeapon::FindAndPathToWeapon( CNEOBot *me )
{
	if ( !m_hTargetWeapon )
	{
		m_hTargetWeapon = FindNearestPrimaryWeapon( me, false, m_pIgnoredWeapons );
	}
	
	if ( !m_hTargetWeapon )
	{
		// no weapon found
		m_path.Invalidate();
	}
	else if ( !PathToTargetWeapon( me ) )
	{
		IgnoreTargetWeapon();
		m_hTargetWeapon = nullptr;
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

	// Check the path before a ghost carrier drops the ghost for this weapon
	if ( !PathToTargetWeapon( me ) )
	{
		IgnoreTargetWeapon();
		return Done( "No full path to the weapon" );
	}
	m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

	auto *pNeoPrimary = assert_cast<CNEOBaseCombatWeapon *>( pPrimary );
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
		IgnoreTargetWeapon();
		return Done("Gave up seeking weapon");
	}

	if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
	{
		if ( !PathToTargetWeapon( me ) )
		{
			IgnoreTargetWeapon();
			return Done( "No full path to the weapon" );
		}
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	if (!m_path.IsValid())
	{
		return Done("Path to weapon is invalid");
	}
	
	// Verify the path actually reaches the weapon
	const float flMaxEndpointDistSqr = 150.0f * 150.0f;
	if ( m_path.GetEndPosition().DistToSqr( m_hTargetWeapon->GetAbsOrigin() ) > flMaxEndpointDistSqr )
	{
		IgnoreTargetWeapon();
		return Done("Weapon is unreachable (path doesn't get close enough)");
	}

	m_path.Update(me);

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot(0);
	if ( pPrimary )
	{
		int myPrefRank = BOT_WEP_PREF_RANK_UNPREFERRED;
		auto *pMyNeoWep = assert_cast<CNEOBaseCombatWeapon *>( pPrimary );
		myPrefRank = GetBotWeaponPreferenceRank( me, pMyNeoWep->GetNeoWepBits() );

		auto *pTargetWeapon = m_hTargetWeapon ? m_hTargetWeapon->MyCombatWeaponPointer() : nullptr;
		if ( !pTargetWeapon )
		{
			return Done( "Target weapon is invalid or not a NEO weapon" );
		}
		auto *pTargetNeoWep = assert_cast<CNEOBaseCombatWeapon *>( pTargetWeapon );

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
	IgnoreTargetWeapon();
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
	IgnoreTargetWeapon();
	m_hTargetWeapon = nullptr;
	m_repathTimer.Invalidate();
	FindAndPathToWeapon(me);
	return TryContinue();
}
