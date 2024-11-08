#pragma once

#include "cbase.h"
#include "baseentity.h"
#include "neo_gamerules.h"

class CNEOGameConfig : public CLogicalEntity
{
	DECLARE_CLASS(CNEOGameConfig, CBaseEntity);
	DECLARE_DATADESC();

public:
	int m_GameType = NEO_GAME_TYPE_TDM;
};
