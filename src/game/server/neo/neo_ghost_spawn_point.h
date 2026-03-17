#ifndef NEO_GHOST_SPAWN_POINT_H
#define NEO_GHOST_SPAWN_POINT_H
#ifdef _WIN32
#pragma once
#endif

class CNEOObjectiveSpawnPoint : public CPointEntity
{
	DECLARE_CLASS(CNEOObjectiveSpawnPoint, CPointEntity);
	DECLARE_DATADESC();

protected:
	void InputEnable(inputdata_t &inputdata) { m_bDisabled = false; }
	void InputDisable(inputdata_t &inputdata) { m_bDisabled = true; }

	bool m_bStartDisabled = false;

public:
	virtual void Spawn() override;

	bool m_bDisabled = false;
	COutputEvent m_OnSpawnedHere;
};

class CNEOGhostSpawnPoint : public CNEOObjectiveSpawnPoint
{
	DECLARE_CLASS(CNEOGhostSpawnPoint, CNEOObjectiveSpawnPoint);
public:
	virtual void Spawn() override;
};

class CNEOJuggernautSpawnPoint : public CNEOObjectiveSpawnPoint
{
	DECLARE_CLASS(CNEOJuggernautSpawnPoint, CNEOObjectiveSpawnPoint);
public:
	virtual void Spawn() override;
};

#endif // NEO_GHOST_SPAWN_POINT_H
