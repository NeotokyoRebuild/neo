#include "c_buttons.h"

#include "tier0/memdbgon.h"

#define SF_BUTTON_USE_ACTIVATES			1024	// Button fires when used.

LINK_ENTITY_TO_CLASS(func_button, C_BaseButton);

IMPLEMENT_CLIENTCLASS_DT( C_BaseButton, DT_BaseButton, CBaseButton )
	RecvPropInt( RECVINFO(m_spawnflags) ),
END_RECV_TABLE()

int C_BaseButton::ObjectCaps(void) {
	return BaseClass::ObjectCaps() | ((m_spawnflags & SF_BUTTON_USE_ACTIVATES) ? (FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS) : 0);
};