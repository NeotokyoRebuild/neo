#pragma once
#include "cbase.h"
#include "utlvector.h"
#include "neo_gamerules.h"  // для enum KothControllingTeams (KOTH_NONE/BOTH/NSF/JINRAI)

class CNEO_Player;

class CNEO_InfoKOTHZone : public CPointEntity
{
public:
	DECLARE_CLASS(CNEO_InfoKOTHZone, CPointEntity);
	DECLARE_DATADESC();

	virtual void Spawn() override;
	virtual void Think() override;

	// called by every neo_trigger_koth_zone referring to this zone
	void OnPlayerEnter(CNEO_Player *pPlayer);
	void OnPlayerLeave(CNEO_Player *pPlayer);

	KothControllingTeams GetState() const { return m_State; }

private:
	void UpdateState();
	// safety net: drops captors that died/disconnected without a matching EndTouch
	void PruneStaleCaptors();

	struct ZoneCaptor
	{
		EHANDLE hPlayer;
		int team;        // team recorded at the moment they entered (survives team switch/death before leaving)
		int touchCount;  // how many of this zone's triggers currently touch them
	};

	CUtlVector<ZoneCaptor> m_Captors;
	int m_iJinraiCount = 0;
	int m_iNSFCount = 0;
	KothControllingTeams m_State = KOTH_NONE;
};

LINK_ENTITY_TO_CLASS(neo_info_koth_zone, CNEO_InfoKOTHZone);
