#include "neo_trigger_koth_zone.h"
#include "neo_gamerules.h"

//##############################
//  Trigger koth zone
//##############################

LINK_ENTITY_TO_CLASS(neo_trigger_koth_zone, CNEO_TriggerKOTHZone);

BEGIN_DATADESC(CNEO_TriggerKOTHZone)
	DEFINE_KEYFIELD(m_iszZoneName, FIELD_STRING, "zone_name"),
END_DATADESC()

void CNEO_TriggerKOTHZone::Spawn()
{
	BaseClass::Spawn();
	InitTrigger();
}

void CNEO_TriggerKOTHZone::Activate()
{
	BaseClass::Activate();

	// resolved here (not in Spawn) since the target neo_info_koth_zone
	// is not guaranteed to have spawned yet while we're still spawning
	pZone = dynamic_cast<CNEO_InfoKOTHZone *>(
		gEntList.FindEntityByName(
			nullptr, STRING(m_iszZoneName)
		)
	);

	if (pZone == nullptr)
	{
		// we SHOULD connect each trigger to some _info_ entity
		Warning("Some neo_trigger_koth_zone was not connected to any neo_info_koth_zone entity!\n");
	}
	else
	{
		// it will be inactive until neo_koth_master enables it
		Disable();
	}
}

void CNEO_TriggerKOTHZone::StartTouch(CBaseEntity *pOther)
{
	CNEO_Player *pPlayer = ToNEOPlayer(pOther);
	if (!pPlayer || !pZone)
		return;

	pZone->OnPlayerEnter(pPlayer);
}

void CNEO_TriggerKOTHZone::EndTouch(CBaseEntity *pOther)
{
	CNEO_Player *pPlayer = ToNEOPlayer(pOther);
	// if player is dead we still need to update zone
	if (!pPlayer || !pZone)
		return;

	pZone->OnPlayerLeave(pPlayer);
}
