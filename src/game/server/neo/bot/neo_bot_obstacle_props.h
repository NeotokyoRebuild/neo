#pragma once

class INextBot;
class CBaseEntity;

// Floor around the path and the props that a detour may use reaches this far past them
constexpr float PROP_DETOUR_GRID_MARGIN = 48.0f;

// A detour stays on floor this close in height to the bot's feet or to where it rejoins the path
constexpr float PROP_DETOUR_FLOOR_HEIGHT_TOLERANCE = 40.0f;

// What a count of the props resting in a region says when one of them is awake
constexpr int PROP_DETOUR_PROPS_MOVING = -1;

//----------------------------------------------------------------------------------------------------------------
// A movable prop in the bot's body space over the floor, or soon there
struct PropObstacle_t
{
	CBaseEntity *entity;
	Vector sweep;			// how far it moves within PROP_DETOUR_MOTION_PREDICT_TIME
	Vector2D lo, hi;		// footprint over that time, grown by a hull half-width
	bool isPushable;		// light enough to shove aside
};

// The bot's body as a box: from a step over the floor to its standing height
struct BodyBox_t
{
	Vector mins, maxs;
};

BodyBox_t GetBodyBox( INextBot *bot );

// Return true if a body moving from 'from' to 'to' touches the prop, now or where it will be
bool BodyMeetsProp( const BodyBox_t &body, const Vector &from, const Vector &to, const PropObstacle_t &obstacle, float *fraction );

// Every movable prop solid to players around the floor in the box, from below it up to the headroom above it,
// and, given a bot and a list for them, the solid entities it can break there
void FindMovableProps( const Vector &floorLo, const Vector &floorHi, CUtlVector< CBaseEntity * > *props,
	INextBot *bot = NULL, CUtlVector< CBaseEntity * > *breakables = NULL );

// Movable props the bot can see that stand, or will soon stand, in its body space over the floor in the box
// and, given a list for them, the breakables there
void CollectProps( INextBot *bot, const Vector &floorLo, const Vector &floorHi, CUtlVector< PropObstacle_t > *obstacles,
	CUtlVector< CBaseEntity * > *breakables = NULL );

// How many movable props stand around the floor in the box, or PROP_DETOUR_PROPS_MOVING if any of them is awake:
// physics puts a prop to sleep once it comes to rest, and wakes it when it is touched or moved
int CountRestingProps( const Vector &floorLo, const Vector &floorHi );
