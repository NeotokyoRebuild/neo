#include "neo_version_info.h"

#include "convar.h"
#include "dbg.h"

void NeoVersionPrint()
{
#if defined(GAME_DLL)
	static constexpr char HEADER[] = "neo_version (Server's build info):";
#elif defined(CLIENT_DLL)
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

namespace {
#ifdef CLIENT_DLL
constexpr int NEO_VERSION_FLAGS = 0;
#else
constexpr int NEO_VERSION_FLAGS = FCVAR_HIDDEN;
#endif
ConCommand neo_version("neo_version", NeoVersionPrint, "Print out client/server's build's information.", NEO_VERSION_FLAGS);

#ifdef CLIENT_DLL
ConVar __neo_cl_git_hash("__neo_cl_git_hash", GIT_LONGHASH,
						 FCVAR_USERINFO | FCVAR_HIDDEN | FCVAR_DONTRECORD | FCVAR_NOT_CONNECTED
#ifndef DEBUG
						 | FCVAR_PRINTABLEONLY
#endif
						 );
#endif
}

#if defined(CLIENT_DLL) && defined(DEBUG)
void InitializeDbgNeoClGitHashEdit()
{
	static char static_dbgHash[sizeof(GIT_LONGHASH) + 1];
	V_strcpy_safe(static_dbgHash, GIT_LONGHASH);
	static_dbgHash[0] = static_dbgHash[0] | 0b1000'0000;
	__neo_cl_git_hash.SetDefault(static_dbgHash);
	__neo_cl_git_hash.Revert(); // It just sets to the default
}
#endif
