#include "cbase.h"
#include "neo_ads_optic.h"
#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool NeoAdsInThermals(const C_NEO_Player *pPlayer)
{
	return pPlayer && pPlayer->GetClass() == NEO_CLASS_SUPPORT && pPlayer->IsInVision();
}
