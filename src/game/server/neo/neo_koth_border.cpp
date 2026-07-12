#include "neo_koth_border.h"
#include "neo_info_koth_zone.h"

BEGIN_DATADESC(CNEO_KOTHBorder)
	DEFINE_KEYFIELD(m_iszZoneName, FIELD_STRING, "zone_name"),
END_DATADESC()

void CNEO_KOTHBorder::Spawn()
{
	// some obvious settings
	AddSpawnFlags(SF_IGNORE_PLAYERUSE);
	m_iSolidity = BRUSHSOLID_NEVER;

	BaseClass::Spawn();

	AddEffects(EF_NOSHADOW);
}

void CNEO_KOTHBorder::Activate()
{
	BaseClass::Activate();

	pZone = dynamic_cast<CNEO_InfoKOTHZone *>(
		gEntList.FindEntityByName(nullptr, STRING(m_iszZoneName))
	);

	if (pZone == nullptr)
		Warning("Some neo_func_koth_border was not connected to any neo_info_koth_zone entity!\n");
	else
		pZone->AddChildBorder(this);
}

void CNEO_KOTHBorder::SetZoneColor(KothControllingTeams team)
{
	switch (team)
	{
	case KOTH_JINRAI:
		SetRenderColor(COLOR_NEO_BLUE.r(), COLOR_NEO_BLUE.g(), COLOR_NEO_BLUE.b());
		DevMsg("JINRAI\n");
		break;
	case KOTH_NSF:
		SetRenderColor(COLOR_BLUE.r(), COLOR_BLUE.g(), COLOR_BLUE.b());
		DevMsg("NSF\n");
		break;
	case KOTH_BOTH:
		SetRenderColor(COLOR_RED.r(), COLOR_RED.g(), COLOR_RED.b());
		DevMsg("STALEMATE\n");
		break;
	case KOTH_NONE:
	default:
		SetRenderColor(COLOR_WHITE.r(), COLOR_WHITE.g(), COLOR_WHITE.b());
		DevMsg("EMPTY\n");
		break;
	}
}
