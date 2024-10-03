// nav_path.h
// Navigation Path encapsulation
// Author: Michael S. Booth (mike@turtlerockstudios.com), November 2003
// https://github.com/ValveSoftware/halflife/tree/master/game_shared/bot
// Ported to Source from the HL1 SDK with adjustments for SourceBots by Bitl

#ifndef _NAV_PATH_H_
#define _NAV_PATH_H_

#include "nav_area.h"

class CImprov;

//--------------------------------------------------------------------------------------------------------
/**
 * The CNavPath class encapsulates a path through space
 */
class CNavPath
{
public:
	CNavPath( void )
	{
		m_segmentCount = 0;
	}

	struct PathSegment
	{
		CNavArea *area;											///< the area along the path
		NavTraverseType how;									///< how to enter this area from the previous one
		Vector pos;												///< our movement goal position at this point in the path
		const CNavLadder *ladder;								///< if "how" refers to a ladder, this is it
	};

	const PathSegment * operator[] ( int i ) const	{ return (i >= 0 && i < m_segmentCount) ? &m_path[i] : NULL; }
	const PathSegment *GetSegment( int i ) const	{ return (i >= 0 && i < m_segmentCount) ? &m_path[i] : NULL; }
	int GetSegmentCount( void ) const				{ return m_segmentCount; }
	const Vector &GetEndpoint( void ) const			{ return m_path[ m_segmentCount-1 ].pos; }
	bool IsAtEnd( const Vector &pos ) const;					///< return true if position is at the end of the path

	float GetLength( void ) const;								///< return length of path from start to finish
	bool GetPointAlongPath( float distAlong, Vector *pointOnPath ) const;	///< return point a given distance along the path - if distance is out of path bounds, point is clamped to start/end

	/// return the node index closest to the given distance along the path without going over - returns (-1) if error
	int GetSegmentIndexAlongPath( float distAlong ) const;

	bool IsValid( void ) const		{ return (m_segmentCount > 0); }
	void Invalidate( void )			{ m_segmentCount = 0; m_bCanReach = true; m_Timer.Invalidate(); }
	bool IsUnreachable() const 		{ return !m_bCanReach; }
	float GetElapsedTimeSinceBuild() const { return m_Timer.GetElapsedTime(); }

	void Draw( void );											///< draw the path for debugging

	/// compute closest point on path to given point
	bool FindClosestPointOnPath( const Vector *worldPos, int startIndex, int endIndex, Vector *close ) const;

	void Optimize( void );
	
	/**
	 * Compute shortest path from 'start' to 'goal' via A* algorithm
	 */
	template< typename CostFunctor >
	bool Compute( const Vector &start, const Vector &goal, CostFunctor &costFunc )
	{
		Invalidate();

		if (start == NULL || goal == NULL)
			return false;

		CNavArea *startArea = TheNavMesh->GetNearestNavArea(start + Vector(0.0f,0.0f,1.0f));
		if (startArea == NULL)
			return false;

		CNavArea *goalArea = TheNavMesh->GetNavArea( goal );

		// if we are already in the goal area, build trivial path
		if (startArea == goalArea)
		{
			BuildTrivialPath( start, goal );
			return true;
		}

		// make sure path end position is on the ground
		Vector pathEndPosition = goal;
		if (goalArea)
			pathEndPosition.z = goalArea->GetZ( &pathEndPosition );
		else
			TheNavMesh->GetGroundHeight( pathEndPosition, &pathEndPosition.z );

		//
		// Compute shortest path to goal
		//
		CNavArea *closestArea;
		bool pathToGoalExists = NavAreaBuildPath( startArea, goalArea, &goal, costFunc, &closestArea );

		m_Timer.Start();
        m_bCanReach = pathToGoalExists;

		//
		// Build path by following parent links
		//

		// get count
		int count = 0;
		CNavArea *area;
		for( area = closestArea; area; area = area->GetParent() )
			++count;

		// save room for endpoint
		if (count > MAX_PATH_SEGMENTS-1)
			count = MAX_PATH_SEGMENTS-1;

		if (count == 0)
			return false;

		if (count == 1)
		{
			BuildTrivialPath( start, goal );
			return true;
		}

		// build path
		m_segmentCount = count;
		for( area = closestArea; count && area; area = area->GetParent() )
		{
			--count;
			m_path[ count ].area = area;
			m_path[ count ].how = area->GetParentHow();
		}

		// compute path positions
		if (ComputePathPositions(start) == false)
		{
			//PrintIfWatched( "Error building path\n" );
			Invalidate();
			return false;
		}

		// append path end position
		m_path[ m_segmentCount ].area = closestArea;
		m_path[ m_segmentCount ].pos = pathEndPosition;
		m_path[ m_segmentCount ].ladder = NULL;
		m_path[ m_segmentCount ].how = NUM_TRAVERSE_TYPES;
		++m_segmentCount;

		return pathToGoalExists;
	}

private:
	enum { MAX_PATH_SEGMENTS = 256 };
	PathSegment m_path[ MAX_PATH_SEGMENTS ];
	int m_segmentCount;
	bool m_bCanReach;
    IntervalTimer m_Timer;

	bool ComputePathPositions( const Vector &start );				///< determine actual path positions 
	bool BuildTrivialPath( const Vector &start, const Vector &goal );		///< utility function for when start and goal are in the same area

	int FindNextOccludedNode( int anchor );		///< used by Optimize()
};

//--------------------------------------------------------------------------------------------------------
/**
 * Monitor improv movement and determine if it becomes stuck
 */
class CStuckMonitor
{
public:
	CStuckMonitor( void );

	void Reset( void );
	void Update( CImprov *improv );
	bool IsStuck( void ) const			{ return m_isStuck; }

	float GetDuration( void ) const	{ return (m_isStuck) ? m_stuckTimer.GetElapsedTime() : 0.0f; }

private:
	bool m_isStuck;													///< if true, we are stuck
	Vector m_stuckSpot;												///< the location where we became stuck
	IntervalTimer m_stuckTimer;										///< how long we have been stuck

	enum { MAX_VEL_SAMPLES = 5 };
	float m_avgVel[ MAX_VEL_SAMPLES ];
	int m_avgVelIndex;
	int m_avgVelCount;
	Vector m_lastCentroid;
	float m_lastTime;
};

//--------------------------------------------------------------------------------------------------------
/**
 * The CNavPathFollower class implements path following behavior
 */
class CNavPathFollower
{
public:
	CNavPathFollower( void );

	void SetImprov( CImprov *improv ) { m_improv = improv; }
	void SetPath( CNavPath *path ) { m_path = path; }
	void SetFollowPathExactly( bool value ) { m_bShouldFollowPathExactly = value; }

	void Reset( void );

	#define DONT_AVOID_OBSTACLES false
	void Update( float deltaT, bool avoidObstacles = true );		///< move improv along path
	void Debug( bool status )		{ m_isDebug = status; }			///< turn debugging on/off

	bool IsStuck( void ) const		{ return m_stuckMonitor.IsStuck(); }	///< return true if improv is stuck 
	void ResetStuck( void )			{ m_stuckMonitor.Reset(); }
	float GetStuckDuration( void ) const	{ return m_stuckMonitor.GetDuration(); }	///< return how long we've been stuck

	void FeelerReflexAdjustment( Vector *goalPosition, float height = -1.0f );	///< adjust goal position if "feelers" are touched

private:
	CImprov *m_improv;												///< who is doing the path following

	CNavPath *m_path;												///< the path being followed

	int m_segmentIndex;												///< the point on the path the improv is moving towards
	int m_behindIndex;												///< index of the node on the path just behind us
	Vector m_goal;													///< last computed follow goal

	bool m_isLadderStarted;

	bool m_isDebug;
	bool m_bShouldFollowPathExactly;

	int FindOurPositionOnPath( Vector *close, bool local ) const;	///< return the closest point to our current position on current path
	int FindPathPoint( float aheadRange, Vector *point, int *prevIndex );	///< compute a point a fixed distance ahead along our path.

public:
	CStuckMonitor m_stuckMonitor;
};



#endif	// _NAV_PATH_H_

