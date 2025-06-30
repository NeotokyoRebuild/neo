#pragma once

#include "NextBot/NextBotEventResponderInterface.h"

class CNEOBot;

class CNEOBotSquad : public INextBotEventResponder
{
public:
	CNEOBotSquad( void );
	virtual ~CNEOBotSquad() { }		

	// EventResponder ------
	virtual INextBotEventResponder *FirstContainedResponder( void ) const override;
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const override;
	//----------------------

	bool IsMember( CNEOBot *bot ) const;		// is the given bot in this squad?
	bool IsLeader( CNEOBot *bot ) const;		// is the given bot the leader of this squad?

// 	CNEOBot *GetMember( int i );
 	int GetMemberCount( void ) const;

	CNEOBot *GetLeader( void ) const;

	class Iterator
	{
	public:
		Iterator( void )
		{
			m_bot = NULL;
			m_index = -1;
		}

		Iterator( CNEOBot *bot, int index )
		{
			m_bot = bot;
			m_index = index;
		}

		CNEOBot *operator() ( void )
		{
			return m_bot;
		}

		bool operator==( const Iterator &it ) const	{ return m_bot == it.m_bot && m_index == it.m_index; }
		bool operator!=( const Iterator &it ) const	{ return m_bot != it.m_bot || m_index != it.m_index; }

		CNEOBot *m_bot;
		int m_index;
	};

	Iterator GetFirstMember( void ) const;
	Iterator GetNextMember( const Iterator &it ) const;
	Iterator InvalidIterator() const;

	void CollectMembers( CUtlVector< CNEOBot * > *memberVector ) const;

	#define EXCLUDE_LEADER false
	float GetSlowestMemberSpeed( bool includeLeader = true ) const;
	float GetSlowestMemberIdealSpeed( bool includeLeader = true ) const;
	float GetMaxSquadFormationError( void ) const;

	bool ShouldSquadLeaderWaitForFormation( void ) const;		// return true if the squad leader needs to wait for members to catch up, ignoring those who have broken ranks
	bool IsInFormation( void ) const;						// return true if the squad is in formation (everyone is in or nearly in their desired positions)

	float GetFormationSize( void ) const;
	void SetFormationSize( float size );

	void DisbandAndDeleteSquad( void );

	void SetShouldPreserveSquad( bool bShouldPreserveSquad ) { m_bShouldPreserveSquad = bShouldPreserveSquad; }
	bool ShouldPreserveSquad() const { return m_bShouldPreserveSquad; }

private:
	friend class CNEOBot;

	void Join( CNEOBot *bot );
	void Leave( CNEOBot *bot );

	CUtlVector< CHandle< CNEOBot > > m_roster;
	CHandle< CNEOBot > m_leader;

	float m_formationSize;
	bool m_bShouldPreserveSquad;
};

inline bool CNEOBotSquad::IsMember( CNEOBot *bot ) const
{
	return m_roster.HasElement( bot );
}

inline bool CNEOBotSquad::IsLeader( CNEOBot *bot ) const
{
	return m_leader == bot;
}

inline CNEOBotSquad::Iterator CNEOBotSquad::InvalidIterator() const
{
	return Iterator( NULL, -1 );
}

inline float CNEOBotSquad::GetFormationSize( void ) const
{
	return m_formationSize;
}

inline void CNEOBotSquad::SetFormationSize( float size )
{
	m_formationSize = size;
}
