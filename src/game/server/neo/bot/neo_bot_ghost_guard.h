#pragma once

class CNavArea;
class CNEOBot;
class CNEO_Player;

// ATK defenders with no capture zone of their own keep the ghost instead of carrying it to a zone
bool IsGhostGuardRound( int team );

// Of the nav areas around the carrier that no teammate has claimed or holds nearby, the one nearest to me,
// or the least crowded one when every candidate is taken
CNavArea *FindGuardArea( CNEOBot *me, CNavArea *pCenterArea );

// Guards tell each other where they hold, so the next guard picks a spot away from them
void SetGuardSpot( CNEOBot *me, CNavArea *pArea );
void ClearGuardSpot( CNEOBot *me );

// The carrier of a guard round walks to a less exposed area: from its area to the area it can see within a step
// with the fewest potentially visible areas, no closer to the attackers' zones, until no visible area is lower
CNavArea *FindLeastExposedArea( CNEOBot *me, CNavArea *pStartArea );

// The carrier tells its guards where it will hold, so they pick spots around it and not along its way
void SetGhostHoldArea( CNEOBot *pCarrier, CNavArea *pArea );
CNavArea *GetGhostHoldArea( const CNEO_Player *pCarrier );
