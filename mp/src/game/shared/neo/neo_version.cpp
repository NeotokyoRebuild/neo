#include "neo_version_info.h"

#include "convar.h"
#include "dbg.h"

namespace {
void neoVersionCallback()
{
#if defined(GAME_DLL)
	static constexpr char HEADER[] = "neo_sv_version (Server's build info):";
#else defined(CLIENT_DLL)
	static constexpr char HEADER[] = "neo_version (Client's build info):";
#endif
	Msg("%s\n"
		"Build version: %s_%s\n"
		"Build datetime: %s\n"
		"Git hash: %s\n"
		"OS: %s\n"
		"Compiler: %s %s\n",
		HEADER,
		BUILD_DATE, GIT_HASH,
		BUILD_DATETIME,
		GIT_LONGHASH,
		OS_BUILD,
		COMPILER_ID, COMPILER_VERSION);
}

#ifdef GAME_DLL
ConCommand neo_version("neo_sv_version", neoVersionCallback, "Print out server's build's information.", FCVAR_GAMEDLL);
#endif
#ifdef CLIENT_DLL
ConCommand neo_version("neo_version", neoVersionCallback, "Print out client's build's information.", FCVAR_CLIENTDLL);
#endif
}
