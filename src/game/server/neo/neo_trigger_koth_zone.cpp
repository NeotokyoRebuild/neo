#include "neo_trigger_koth_zone.h"
#include "neo_gamerules.h"


#define THINK_INTERVAL 0.05f

//##############################
//  Trigger koth zone
//##############################

LINK_ENTITY_TO_CLASS(neo_trigger_koth_zone, CNEO_TriggerKOTHZone);

BEGIN_DATADESC(CNEO_TriggerKOTHZone)
	DEFINE_THINKFUNC(Think),
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
	else {
		// it will be inactive
		SetThink( &CNEO_TriggerKOTHZone::Think );
		SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
		Disable();
	}
}

void CNEO_TriggerKOTHZone::Think()
{
	// do lazy check then send info to related neo_info_koth_zone
	// then send updated info into pZone
	SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
}

void CNEO_TriggerKOTHZone::StartTouch(CBaseEntity *pOther)
{
	CNEO_Player *pPlayer = ToNEOPlayer(pOther);
	if (!pPlayer || !pPlayer->IsAlive())
		return;

	//Msg("player %s is inside!\n", pPlayer->GetPlayerName());
	//... add player locally
}

void CNEO_TriggerKOTHZone::EndTouch(CBaseEntity *pOther)
{
	CNEO_Player *pPlayer = ToNEOPlayer(pOther);
	if (!pPlayer || !pPlayer->IsAlive())
		return;

	//Msg("player %s is inside!\n", pPlayer->GetPlayerName());
	//... rm player locally
}