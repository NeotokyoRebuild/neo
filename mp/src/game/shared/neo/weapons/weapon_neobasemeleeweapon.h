#ifndef WEAPON_NEO_BASEMELEEWEAPON_SHARED_H
#define WEAPON_NEO_BASEMELEEWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_neobaseweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#ifdef CLIENT_DLL
#define CNEOBaseMeleeWeapon C_NEOBaseMeleeWeapon
#endif

abstract_class INEOBaseMeleeWeapon : public INEOBaseWeapon
{
private:
	INEOBaseMeleeWeapon(const INEOBaseMeleeWeapon &other);
};

#endif // WEAPON_NEO_BASEMELEEWEAPON_SHARED_H