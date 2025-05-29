#include "neo_version_info.h"

#include "convar.h"
#include "dbg.h"

#ifdef CLIENT_DLL
#include "materialsystem/imaterialsystem.h"
#endif

void NeoVersionPrint()
{
#if defined(GAME_DLL)
	static constexpr char HEADER[] = "neo_version (Server's build info):";
#elif defined(CLIENT_DLL)
	static constexpr char HEADER[] = "neo_version (Client's build info):";
	const RenderBackend_t eRenderBackend = g_pMaterialSystem->GetRenderBackend();
#endif

	Msg("%s\n"
		"Build version: %s_%s\n"
		"Build date: %s\n"
		"Git hash: %s\n"
		"OS: %s\n"
		"Compiler: %s %s\n"
#ifdef CLIENT_DLL
		"Renderer: %s\n"
#endif
		, HEADER,
		BUILD_DATE_SHORT, GIT_HASH,
		BUILD_DATE_LONG,
		GIT_LONGHASH,
		OS_BUILD,
		COMPILER_ID, COMPILER_VERSION
#ifdef CLIENT_DLL
		, GetRenderBackendName(eRenderBackend)
#endif
		);
}

namespace {
#ifdef CLIENT_DLL
constexpr int NEO_VERSION_FLAGS = 0;
#else
constexpr int NEO_VERSION_FLAGS = FCVAR_HIDDEN;
#endif
ConCommand neo_version("neo_version", NeoVersionPrint, "Print out client/server's build's information.", NEO_VERSION_FLAGS);

#ifdef CLIENT_DLL
ConVar __cl_neo_git_hash("__cl_neo_git_hash", GIT_LONGHASH,
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
	static char static_dbgHash[GIT_LONGHASH_SIZE];
	V_strcpy_safe(static_dbgHash, GIT_LONGHASH);
	static_dbgHash[0] = static_dbgHash[0] | 0b1000'0000;
	__cl_neo_git_hash.SetDefault(static_dbgHash);
	__cl_neo_git_hash.Revert(); // It just sets to the default
}
#endif
