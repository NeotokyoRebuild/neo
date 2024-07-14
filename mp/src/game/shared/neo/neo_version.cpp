#include "neo_version_info.h"

#include "convar.h"
#include "dbg.h"

namespace {
void neoVersionCallback()
{
#if defined(GAME_DLL)
	static constexpr char HEADER[] = "neo_version (Server's build info):";
#else defined(CLIENT_DLL)
	static constexpr char HEADER[] = "neo_version (Client's build info):";
#endif
	Msg("%s\n"
		"Build version: %s_%s\n"
		"Build date: %s\n"
		"Git hash: %s\n"
		"OS: %s\n"
		"Compiler: %s %s\n",
		HEADER,
		BUILD_DATE_SHORT, GIT_HASH,
		BUILD_DATE_LONG,
		GIT_LONGHASH,
		OS_BUILD,
		COMPILER_ID, COMPILER_VERSION);
}

#ifdef CLIENT_DLL
constexpr int NEO_VERSION_FLAGS = 0;
#else
constexpr int NEO_VERSION_FLAGS = FCVAR_HIDDEN;
#endif
ConCommand neo_version("neo_version", neoVersionCallback, "Print out client/server's build's information.", NEO_VERSION_FLAGS);
}
