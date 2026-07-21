#include "c_neo_killer_infos.h"

#include "strtools.h"

void NeoUserIDsLocalKilledClear()
{
	g_neoUserIDsLocalKilledSize = 0;
	V_memset(g_neoUserIDsLocalKilled, 0, sizeof(g_neoUserIDsLocalKilled));
}

