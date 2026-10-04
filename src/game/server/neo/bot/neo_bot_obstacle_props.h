#pragma once

class INextBot;
class CBaseEntity;

// A query for props reaches this far past the floor box on each side,
constexpr float OBSTACLE_PROP_QUERY_MARGIN = 48.0f;
// and this far below its floor
constexpr float OBSTACLE_PROP_FLOOR_TOLERANCE = 40.0f;

// What a count of the props resting in a region says when one of them is awake
constexpr int OBSTACLE_PROPS_MOVING = -1;

//----------------------------------------------------------------------------------------------------------------
// A movable prop in the bot's body space over the floor, or soon there
struct PropObstacle_t
{
	CBaseEntity *entity;
	Vector sweep;			// how far it moves within OBSTACLE_PROP_MOTION_PREDICT_TIME
	Vector2D lo, hi;		// footprint over that time, grown by a hull half-width
	bool isPushable;		// light enough to shove aside
};

// The bot's body as a box: from a step over the floor to its standing height
struct BodyBox_t
{
	Vector mins, maxs;
};

BodyBox_t GetBodyBox( INextBot *bot );

// Return true if a body moving from 'from' to 'to' touches the entity where it stands
bool BodyMeetsEntity( const BodyBox_t &body, const Vector &from, const Vector &to, CBaseEntity *entity, float *fraction );

// Return true if a body moving from 'from' to 'to' touches the prop, now or where it will be
bool BodyMeetsProp( const BodyBox_t &body, const Vector &from, const Vector &to, const PropObstacle_t &obstacle, float *fraction );

// Every movable prop solid to players around the floor in the box, from below it up to the headroom above it
void FindMovableProps( const Vector &floorLo, const Vector &floorHi, CUtlVector< CBaseEntity * > *props );

// Every solid entity the bot can break around the floor in the box, over the same reach
void FindBreakables( INextBot *bot, const Vector &floorLo, const Vector &floorHi, CUtlVector< CBaseEntity * > *breakables );

// Movable props the bot can see that stand, or will soon stand, in its body space over the floor in the box
// and, given a list for them, the breakables there
void CollectProps( INextBot *bot, const Vector &floorLo, const Vector &floorHi, CUtlVector< PropObstacle_t > *obstacles,
	CUtlVector< CBaseEntity * > *breakables = NULL );

// How many movable props stand around the floor in the box, or OBSTACLE_PROPS_MOVING if any of them is awake:
// physics puts a prop to sleep once it comes to rest, and wakes it when it is touched or moved
int CountRestingProps( const Vector &floorLo, const Vector &floorHi );
