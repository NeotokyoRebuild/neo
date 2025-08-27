//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// No spaces in event names, max length 32
// All strings are case sensitive
//
// valid data key types are:
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit
//   local  : any data, but not networked to clients
//
// following key names are reserved:
//   local      : if set to 1, event is not networked to clients
//   unreliable : networked, but unreliable
//   suppress   : never fire this event
//   time	: firing server time
//   eventid	: holds the event ID

"modevents"
{
	"player_death"				// a game event, name may be 32 charaters long
	{
		"userid"	"short"   	// user ID who died				
		"attacker"	"short"	 	// user ID who killed
		"assists"	"short"		// user ID who assists
		"weapon"	"string" 	// weapon name killed used
		"headshot"	"bool"		// killed by headshot
		"suicide"	"bool"		// suicide
		"headshot"	"bool"		// killed by headshot
		"deathIcon"	"string"	// weapon icon
		"explosive"	"bool"		// killed by explosion
		"ghoster"	"bool"		// user who died was ghoster
	}
	"player_rankchange"
	{
		"userid"	"short"		// user who's rank changed
		"oldRank"	"short"
		"newRank"	"short"
	}
	"ghost_capture"
	{
		"userid"	"short"		// user who captured the ghost
	}
	"vip_extract"
	{
		"userid"	"short"		// user who escaped as VIP
	}
	"vip_death"
	{
		"userid"	"short"		// user who died as VIP		
		"attacker"	"short"	 	// user who killed the VIP
	}
	"player_changeneoname"			// player_changename, but for neo_name
	{
		"userid"	"short"   	// user ID who neo_name changed
		"oldname"	"string" 	// user's old name
		"newname"	"string" 	// user's new name
	}
	
	"teamplay_round_start"			// round restart
	{
		"full_reset"	"bool"		// is this a full reset of the map
	}
	
	"spec_target_updated"
	{
	}
	
	"achievement_earned"
	{
		"player"	"byte"		// entindex of the player
		"achievement"	"short"		// achievement ID
	}
	
	"player_ping"
	{
		"userid"		"short"   	// userID of player who pinged
		"playerteam"	"short"		// pinger team
		"pingx"			"short"		// ping x position
		"pingy"			"short"		// ping y position
		"pingz"			"short"		// ping z position
		"ghosterping"	"bool"		// the player is carrying the ghost
	}
	
	// inherited from NT
	"game_round_start"
	{
	
	}
	
	// inherited from NT
	"game_round_end"
	{
	
	}
}
