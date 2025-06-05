#include "cbase.h"

#include "bot/neo_bot.h"
#include "neo_bot_proxy.h"
#include "neo_bot_generator.h"


BEGIN_DATADESC( CNEOBotProxy )
	DEFINE_KEYFIELD( m_botName,				FIELD_STRING,	"bot_name" ),
	DEFINE_KEYFIELD( m_teamName,			FIELD_STRING,	"team" ),
	DEFINE_KEYFIELD( m_respawnInterval,		FIELD_FLOAT,	"respawn_interval" ),
	DEFINE_KEYFIELD( m_actionPointName,		FIELD_STRING,	"action_point" ),
	DEFINE_KEYFIELD( m_spawnOnStart,		FIELD_STRING,	"spawn_on_start" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetTeam", InputSetTeam ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMovementGoal", InputSetMovementGoal ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Spawn", InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Delete", InputDelete ),

	DEFINE_OUTPUT( m_onSpawned, "OnSpawned" ),
	DEFINE_OUTPUT( m_onInjured, "OnInjured" ),
	DEFINE_OUTPUT( m_onKilled, "OnKilled" ),
	DEFINE_OUTPUT( m_onAttackingEnemy, "OnAttackingEnemy" ),
	DEFINE_OUTPUT( m_onKilledEnemy, "OnKilledEnemy" ),

	DEFINE_THINKFUNC( Think ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( bot_proxy, CNEOBotProxy );



//------------------------------------------------------------------------------
CNEOBotProxy::CNEOBotProxy( void )
{
	V_strcpy_safe( m_botName, "NEOBot" );
	V_strcpy_safe( m_teamName, "auto" );
	m_bot = NULL;
	m_moveGoal = NULL;
	SetThink( NULL );
}


//------------------------------------------------------------------------------
void CNEOBotProxy::Think( void )
{

}


//------------------------------------------------------------------------------
void CNEOBotProxy::InputSetTeam( inputdata_t &inputdata )
{
	const char *teamName = inputdata.value.String();
	if ( teamName && teamName[0] )
	{
		V_strcpy_safe( m_teamName, teamName );

		// if m_bot exists, tell it to change team
		if ( m_bot != NULL )
		{
			m_bot->HandleCommand_JoinTeam( Bot_GetTeamByName( m_teamName ) );
		}
	}
}


//------------------------------------------------------------------------------
void CNEOBotProxy::InputSetMovementGoal( inputdata_t &inputdata )
{
	const char *entityName = inputdata.value.String();
	if ( entityName && entityName[0] )
	{
		m_moveGoal = dynamic_cast< CNEOBotActionPoint * >( gEntList.FindEntityByName( NULL, entityName ) );

		// if m_bot exists, tell it to move to the new action point
		if ( m_bot != NULL )
		{
			m_bot->SetActionPoint( (CNEOBotActionPoint *)m_moveGoal.Get() );
		}
	}
}


//------------------------------------------------------------------------------
void CNEOBotProxy::InputSpawn( inputdata_t &inputdata )
{
	m_bot = NextBotCreatePlayerBot< CNEOBot >( m_botName );
	if ( m_bot != NULL )
	{
		m_bot->SetSpawnPoint( this );
		m_bot->SetAttribute( CNEOBot::REMOVE_ON_DEATH );
		m_bot->SetAttribute( CNEOBot::IS_NPC );

		m_bot->SetActionPoint( (CNEOBotActionPoint *)m_moveGoal.Get() );

		m_bot->HandleCommand_JoinTeam( Bot_GetTeamByName( m_teamName ) );

		m_onSpawned.FireOutput( m_bot, m_bot );
	}
}


//------------------------------------------------------------------------------
void CNEOBotProxy::InputDelete( inputdata_t &inputdata )
{
	if ( m_bot != NULL )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", m_bot->GetUserID() ) );
		m_bot = NULL;
	}
}


//------------------------------------------------------------------------------
void CNEOBotProxy::OnInjured( void )
{
	m_onInjured.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CNEOBotProxy::OnKilled( void )
{
	m_onKilled.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CNEOBotProxy::OnAttackingEnemy( void )
{
	m_onAttackingEnemy.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CNEOBotProxy::OnKilledEnemy( void )
{
	m_onKilledEnemy.FireOutput( this, this );
}

