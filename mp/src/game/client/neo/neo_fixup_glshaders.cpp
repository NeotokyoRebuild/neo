#include "neo_fixup_glshaders.h"
#ifdef LINUX

// Engine
#include "dbg.h"
#include "filesystem.h"
#include "strtools.h"

// Linux
#include <errno.h>
#include <sys/stat.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NEO HACK (Rain): This is a hack around a vision shaders corruption on Linux.
// For details, see: https://github.com/NeotokyoRebuild/neo/pull/587
void FixupGlShaders(IFileSystem* filesystem)
{
	constexpr char filename[] = "glshaders.cfg";
	constexpr auto pathID = "MOD";
	if (filesystem->FileExists(filename, pathID))
	{
		return;
	}

	char path[MAX_PATH]{0};
	filesystem->RelativePathToFullPath("", pathID, path, sizeof(path));
	constexpr int maxPathLen = sizeof(path)-(1+sizeof(filename));
	if (V_strlen(path) >= maxPathLen)
	{
		Warning("%s: Relative path too long to fixup %s: \"%s\"\nExpected max path length of %d, but got %d\n",
				__func__, filename, path, maxPathLen, V_strlen(path));
		return;
	}
	V_strcat_safe(path, filename);

	constexpr const char contents[] = "glshadercachev002\n{\n}";
	auto fh = filesystem->Open(filename, "wt", pathID);
	filesystem->Write(contents, sizeof(contents)-1, fh); // don't write the null terminator to file
	filesystem->Flush(fh);
	filesystem->Close(fh);

	constexpr auto readonly = S_IRUSR|S_IRGRP|S_IROTH;
	if (chmod(path, readonly) == -1)
	{
		Warning("%s: File chmod failed with error code %d\nfor path: \"%s\"\n", __func__, errno, path);
	}
}
#endif // LINUX
