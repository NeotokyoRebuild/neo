#include "neo_fixup_glshaders.h"
#ifdef LINUX

// Engine
#include "filesystem.h"
#include "icvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NEO HACK (Rain): This is a hack around a vision shaders corruption on Linux.
// For details, see: https://github.com/NeotokyoRebuild/neo/pull/587
void FixupGlShaders(IFileSystem* filesystem, ICvar* cvar)
{
	constexpr auto filename = "glshaders.cfg";
	constexpr auto pathID = "MOD";

	// This prevents the game from generating the problematic file on exit.
	// There's also a "mat_autoload_glshaders", but it seems by the time we load,
	// it's too late for that to take an effect, at least via this route.
	// The client can still override this by including a command in their autoexec,
	// if they really want to.
	constexpr auto cvarname = "mat_autosave_glshaders";
	ConVar* autosaveGlShaders;
	if (!(autosaveGlShaders = cvar->FindVar(cvarname)))
	{
		Warning("%s: cvar %s not found\n", __FUNCTION__, cvarname);
		return;
	}
	autosaveGlShaders->SetValue(false);

	// If the problematic file doesn't exist, we're done.
	if (!filesystem->FileExists(filename, pathID))
	{
		return;
	}

	// But if it does, we're too late to fix it for this launch...
	// However, we can delete the file, which together with the convar set above
	// should prevent it from being created again on successive restarts of the game.

	// This should never happen, but just in case the relative mod path lookup somehow fails.
	if (filesystem->IsDirectory(filename, pathID))
	{
		Warning("%s: Expected to find a file at %s path %s, but it was a dir\n",
			__FUNCTION__, pathID, filename);
		return;
	}

	filesystem->RemoveFile(filename, pathID);
}
#endif // LINUX
