#pragma once

#if defined( _X360 )
	#define FORMAT_BSPFILE "maps\\%s.360.bsp"
	#define FORMAT_NAVFILE "maps\\%s.360.nav"
#else
	#define FORMAT_BSPFILE "maps\\%s.bsp"
#ifdef NEO
	#define FORMAT_NAVFILE "maps\\nav\\%s.nav"
#else
	#define FORMAT_NAVFILE "maps\\%s.nav"
#endif // NEO
	#define PATH_NAVFILE_EMBEDDED "maps\\embed.nav"
#endif

#define NAV_MAGIC_NUMBER 0xFEEDFACE				// to help identify nav files

//--------------------------------------------------------------------------------------------------------------
/// The current version of the nav file format

/// IMPORTANT: If this version changes, the swap function in makegamedata 
/// must be updated to match. If not, this will break the Xbox 360.
// TODO: Was changed from 15, update when latest 360 code is integrated (MSB 5/5/09)
const int NavCurrentVersion = 16;

enum NavDirType
{
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	NUM_DIRECTIONS
};

enum { MAX_NAV_TEAMS = 2 };

enum NavCornerType
{
	NORTH_WEST = 0,
	NORTH_EAST = 1,
	SOUTH_EAST = 2,
	SOUTH_WEST = 3,

	NUM_CORNERS
};

#ifdef CLIENT_DLL
// defined in CNavLadder
enum LadderDirectionType
{
	LADDER_UP = 0,
	LADDER_DOWN,

	NUM_LADDER_DIRECTIONS
};
#endif // CLIENT_DLL