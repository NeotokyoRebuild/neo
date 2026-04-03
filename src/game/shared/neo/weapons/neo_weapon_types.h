#pragma once

// NEO NOTE (nullsystem): If this is altered and may need to alter
// for crosshair, check MAP_WEAPON_TYPE_TO_XHAIR in neo_crosshair.h
enum ENeoWeaponType : int
{
	WEP_TYPE_NIL = 0,
	WEP_TYPE_THROWABLE,
	WEP_TYPE_PISTOL,
	WEP_TYPE_SMG,
	WEP_TYPE_SHOTGUN,
	WEP_TYPE_RIFLE,
	WEP_TYPE_MACHINEGUN,
	WEP_TYPE_SNIPER,

	WEP_TYPE__TOTAL,
};

