#include "cbase.h"
#include "neo_hud_childelement.h"

#include "neo_hud_elements.h"
#include "c_neo_player.h"

#include "clientmode_hl2mpnormal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CNEOHud_ChildElement::CNEOHud_ChildElement()
{
	m_pNeoHud = dynamic_cast<CNeoHudElements*>(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_NEO_HUD));
	Assert(m_pNeoHud);

	m_flLastUpdateTime = 0;
}
