#include "cbase.h"
#include "neo_player_spawnpoint.h"

#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNEOSpawnPoint_Jinrai : public CNEOSpawnPoint
{
public:
	CNEOSpawnPoint_Jinrai::CNEOSpawnPoint_Jinrai()
		: CNEOSpawnPoint()
	{
		m_iOwningTeam = TEAM_JINRAI;
	}
};
LINK_ENTITY_TO_CLASS(info_player_attacker, CNEOSpawnPoint_Jinrai);

class CNEOSpawnPoint_NSF : public CNEOSpawnPoint
{
public:
	CNEOSpawnPoint_NSF::CNEOSpawnPoint_NSF()
		: CNEOSpawnPoint()
	{
		m_iOwningTeam = TEAM_NSF;
	}
};
LINK_ENTITY_TO_CLASS(info_player_defender, CNEOSpawnPoint_NSF);

#ifdef GAME_DLL
IMPLEMENT_SERVERCLASS_ST(CNEOSpawnPoint, DT_NEOSpawnPoint)
END_SEND_TABLE()
#else
#ifdef CNEOSpawnPoint
#undef CNEOSpawnPoint
#endif
IMPLEMENT_CLIENTCLASS_DT(C_NEOSpawnPoint, DT_NEOSpawnPoint, CNEOSpawnPoint)
END_RECV_TABLE()
#define CNEOSpawnPoint C_NEOSpawnPoint
#endif

BEGIN_DATADESC(CNEOSpawnPoint)
END_DATADESC()

CNEOSpawnPoint::CNEOSpawnPoint()
{
	m_iOwningTeam = TEAM_UNASSIGNED;
}

CNEOSpawnPoint::~CNEOSpawnPoint()
{
	
}

void CNEOSpawnPoint::Spawn()
{
	BaseClass::Spawn();

	AssertMsg(m_iOwningTeam == TEAM_JINRAI || m_iOwningTeam == TEAM_NSF,
		"CNEOSpawnPoint shouldn't be instantiated directly; use info_player_attacker/defender instead!\n");

#if(0)
	DevMsg("Neo spawnpoint for %s at %f %f %f\n",
		(m_iOwningTeam == TEAM_JINRAI ? "Jinrai" : "NSF"),
		GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
#endif
}
