#include "weapon_bits.h"

#define DEFINE_WEAPON_TYPE(_x1, _x2, _x3, weptype) weptype,
#define DEFINE_WEAPON_TYPE_ALT(_x1, _x2, _x3, _x4, weptype) weptype,

const ENeoWeaponType NEO_WEAPON_TYPE[NEO_WIDX__TOTAL] = {
	WEP_TYPE_NIL, // 0 = invalid, set none
	FOR_LIST_WEAPONS(DEFINE_WEAPON_TYPE, DEFINE_WEAPON_TYPE_ALT)
#ifdef INCLUDE_WEP_PBK
	WEP_TYPE_MACHINEGUN,
#endif
};

