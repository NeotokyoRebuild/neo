#pragma once

class CNavArea;
class CNEOBot;

// ATK defenders with no capture zone of their own keep the ghost instead of carrying it to a zone
bool IsGhostGuardRound( int team );

// The nearest nav area around the carrier that no teammate has claimed or holds nearby,
// or the least crowded one when every candidate is taken
CNavArea *FindGuardArea( CNEOBot *me, CNavArea *pCenterArea );

// Guards tell each other where they hold, so the next guard picks a spot away from them
void SetGuardSpot( CNEOBot *me, CNavArea *pArea );
void ClearGuardSpot( CNEOBot *me );
