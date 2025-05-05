#ifndef NEO_PLAYER_SPAWNPOINT_H
#define NEO_PLAYER_SPAWNPOINT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity_shared.h"
#include "baseplayer_shared.h"

#ifdef CLIENT_DLL
#define CNEOSpawnPoint C_NEOSpawnPoint
#endif

class CNEOSpawnPoint : public CBaseEntity
{
	DECLARE_CLASS(CNEOSpawnPoint, CBaseEntity);

public:
#ifdef GAME_DLL
	DECLARE_SERVERCLASS();
#else
	DECLARE_CLIENTCLASS();
#endif
	DECLARE_DATADESC();

	CNEOSpawnPoint();
	~CNEOSpawnPoint();

	virtual void Spawn();

#ifdef GAME_DLL
	bool m_bDisabled;

	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);

	COutputEvent m_OnPlayerSpawn;
#endif

protected:
	int m_iOwningTeam;

private:
	CNEOSpawnPoint(const CNEOSpawnPoint &other);
};

#endif // NEO_PLAYER_SPAWNPOINT_H