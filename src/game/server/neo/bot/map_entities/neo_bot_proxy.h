#pragma once

class CNEOBot;
class CNEOBotActionPoint;


class CNEOBotProxy : public CPointEntity
{
	DECLARE_CLASS( CNEOBotProxy, CPointEntity );
public:
	DECLARE_DATADESC();

	CNEOBotProxy( void );
	virtual ~CNEOBotProxy() { }

	void Think( void );

	// Input
	void InputSetTeam( inputdata_t &inputdata );
	void InputSetMovementGoal( inputdata_t &inputdata );
	void InputSpawn( inputdata_t &inputdata );
	void InputDelete( inputdata_t &inputdata );

	void OnInjured( void );
	void OnKilled( void );
	void OnAttackingEnemy( void );
	void OnKilledEnemy( void );

protected:
	// Output
	COutputEvent m_onSpawned;
	COutputEvent m_onInjured;
	COutputEvent m_onKilled;
	COutputEvent m_onAttackingEnemy;
	COutputEvent m_onKilledEnemy;

	char m_botName[MAX_PLAYER_NAME_LENGTH];
	char m_teamName[MAX_TEAM_NAME_LENGTH];

	string_t m_spawnOnStart;
	string_t m_actionPointName;
	float m_respawnInterval;

	CHandle< CNEOBot > m_bot;
	CHandle< CNEOBotActionPoint > m_moveGoal;
};
