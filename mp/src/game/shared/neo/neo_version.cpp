#include "neo_version_info.h"

#include "convar.h"
#include "dbg.h"

namespace {
void neoVersionCallback()
{
	Msg("neo_version:\n"
		"Build version: %s_%s\n"
		"Build datetime: %s\n"
		"Git hash: %s\n"
		"OS: %s\n"
		"Compiler: %s %s\n",
		BUILD_DATE, GIT_HASH,
		BUILD_DATETIME,
		GIT_LONGHASH,
		OS_BUILD,
		COMPILER_ID, COMPILER_VERSION);
}

ConCommand neo_version("neo_version", neoVersionCallback, "Print out build's information.");
}
