#ifndef NEO_MOUNT_ORIGINAL_H
#define NEO_MOUNT_ORIGINAL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1.h"
#include "mathlib/vmatrix.h"
#include "shareddefs.h"
#include "steam/steam_api.h"
#include "filesystem.h"

#ifdef LINUX
// User for renaming folder paths.
#include <cstdio>
#include <errno.h>
// Used to determine whether directories exist.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// This needs to start with a -
#define NEO_PATH_PARM_CMD "-neopath"

#define NEO_MOUNT_PATHID "GAME"

#ifdef LINUX
typedef struct stat StatStruct;
static inline int Stat(const char* path, StatStruct* buf) { return stat(path, buf); }

static inline bool IsDir(const StatStruct& st) { return S_ISDIR(st.st_mode); }

static inline bool DirExists(const char *path)
{
    StatStruct file_stat;
    return (Stat(path, &file_stat) == 0 && IsDir(file_stat));
}
#else
// Attempts to build a valid NT game path using the provided Neo path, and Steam info.
// Returns true/false depending on if we successfully found a valid path.
// NOTE: this *will* modify the input path pointer, regardless of success!!
bool IsNeoGameInfoPathOK(char *out_neoPath, const int pathLen)
{
	// This only works for Steam users, and should only be called for them.
	if (!SteamAPI_IsSteamRunning())
	{
		Assert(false);
		return false;
	}

	// Get Steam path
	V_strncpy(out_neoPath, SteamAPI_GetSteamInstallPath(), pathLen);
	V_AppendSlash(out_neoPath, pathLen);

	// Try and see if we have NT installed under the base Steam path
	V_strcat(out_neoPath, "steamapps\\common\\NEOTOKYO\\NeotokyoSource", pathLen);
	if (filesystem->IsDirectory(out_neoPath))
	{
		// Make sure there's actually a GameInfo.txt in the path, otherwise we will crash on mount.
		// We make the assumption that any GameInfo.txt found will be valid format.
		char gameInfoPath[MAX_PATH];
		V_strcpy_safe(gameInfoPath, out_neoPath);
		V_AppendSlash(gameInfoPath, sizeof(gameInfoPath));
		V_strcat(gameInfoPath, "GameInfo.txt", sizeof(gameInfoPath));

		if (filesystem->FileExists(gameInfoPath))
		{
			return true;
		}
	}

	// Otherwise, look up additional Steam library folders elsewhere
	char libraryFoldersPath[MAX_PATH];
	V_strcpy_safe(libraryFoldersPath, SteamAPI_GetSteamInstallPath());
	V_AppendSlash(libraryFoldersPath, sizeof(libraryFoldersPath));
	V_strcat(libraryFoldersPath, "steamapps\\libraryfolders.vdf", sizeof(libraryFoldersPath));

	bool bGameInfoPathIsOK = false;
	KeyValues *pKvLibFolders = new KeyValues("LibraryFolders");
	if (pKvLibFolders->LoadFromFile(filesystem, libraryFoldersPath))
	{
		const int maxExtraLibs = 50;

		for (int i = 1; i <= maxExtraLibs; i++)
		{
			char libStr[3];
			itoa(i, libStr, sizeof(libStr));

			KeyValues *pKvLib = pKvLibFolders->FindKey(libStr);
			if (!pKvLib)
			{
				break;
			}

			const char *libPath = pKvLib->GetString();
			if (*libPath != NULL && filesystem->IsDirectory(libPath))
			{
				V_strncpy(out_neoPath, libPath, pathLen);
				V_AppendSlash(out_neoPath, pathLen);
				V_strcat(out_neoPath, "steamapps\\common\\NEOTOKYO\\NeotokyoSource", pathLen);

				if (filesystem->IsDirectory(out_neoPath))
				{
					// Make sure there's actually a GameInfo.txt in the path, otherwise we will crash on mount.
					// We make the assumption that any GameInfo.txt found will be valid format.
					char gameInfoPath[MAX_PATH];
					V_strcpy_safe(gameInfoPath, out_neoPath);
					V_AppendSlash(gameInfoPath, sizeof(gameInfoPath));
					V_strcat(gameInfoPath, "GameInfo.txt", sizeof(gameInfoPath));

					if (filesystem->FileExists(gameInfoPath))
					{
						bGameInfoPathIsOK = true;
						break;
					}
				}
			}
		}
	}
	pKvLibFolders->deleteThis();

	return bGameInfoPathIsOK;
}
#endif

#ifdef LINUX
// Purpose: Rename specific problematic NT paths to lowercase where their
// inconsistent capitalization causes issues for Linux filesystems (EXT etc).
//
// This is kind of kludgy, but it's the most straightforward way of handling this
// for soundscripts due to how they are read from the original files. It's also
// Windows (NTFS) compatible, since Windows is case insensitive unlike most Linux FS's.
//
// NEO TODO (Rain): We should probably use a pop up dialog to confirm with the user,
// but leaving it out for now since this rename has practically no side effect.
static inline void FixCaseSensitivePathsForLinux(const char *neoPath)
{
    const char *szThisCaller = "FixCaseSensitivePathsForLinux";

    Msg("%s: Checking for Neotokyo path case sensitivity issues...\n", szThisCaller);

    const char *szPathsToFix[] = {
        "sound/weapons/Jitte/",
    };

    int numSuccesses = 0;
    for (int i = 0; i < ARRAYSIZE(szPathsToFix); i++)
    {
        char szLowerCaseTarget[MAX_PATH] { 0 };
        char szOriginalCaseBuffer[MAX_PATH] { 0 };
        char szResultBuffer[MAX_PATH] { 0 };

        V_strcpy_safe(szLowerCaseTarget, szPathsToFix[i]);
        V_strlower(szLowerCaseTarget);

        V_sprintf_safe(szOriginalCaseBuffer, "%s%s", neoPath, szPathsToFix[i]);
        V_sprintf_safe(szResultBuffer, "%s%s", neoPath, szLowerCaseTarget);

        if (DirExists(szOriginalCaseBuffer))
        {
            if (DirExists(szResultBuffer))
            {
                Warning("%s: Both lower and upper case versions of this path exist at once: %s\n",
                    szThisCaller, szOriginalCaseBuffer);
                continue;
            }
        }
        else
        {
            if (DirExists(szResultBuffer))
            {
                ++numSuccesses;
            }
            else
            {
                Warning("%s: Could not find path: %s\n", szThisCaller, szResultBuffer);
            }

            continue;
        }

        Msg("%s: For Linux mixed case filesystem compatibility, now attempting to rename this folder path \
to partially lowercase:\n\"%s\" --> \"%s\"\n",
            szThisCaller, szOriginalCaseBuffer, szResultBuffer);

        if (rename(szOriginalCaseBuffer, szResultBuffer) == 0)
        {
            Msg("%s: Path renaming was successful; result: \"%s\"\n", szThisCaller, szResultBuffer);
            ++numSuccesses;
        }
        else
        {
            Warning("%s: Path renaming failed for: %s\n", szThisCaller, strerror(errno));
        }
    }

    const bool allPathsOk = (numSuccesses == ARRAYSIZE(szPathsToFix));

    if (allPathsOk)
    {
        Msg("%s: All OK; scanned Neotokyo folder path cases are Linux compatible.\n", szThisCaller);
    }
    else
    {
        Warning("%s: Some folder path cases are not Linux compatible, this may cause errors! \
See the error lines above for more info.\n", szThisCaller);
    }
}
#endif

// Either modify Neotokyo assets upon mount to fix incompatibilities,
// or restore those modifications by setting the restoreInstead boolean.
// Will warn on failed initial modification, and error out if the restoration
// of assets failed for whatever reason.
void FixIncompatibleNeoAssets(IFileSystem* filesystem, const char* neoPath, bool restoreInstead)
{
	const char* szThisCaller = "FixIncompatibleNeoAssets";

	Assert(neoPath != NULL && strlen(neoPath) > 0);

	if (!filesystem)
	{
		Assert(false);
		Error("%s: filesystem was NULL\n", szThisCaller);
	}

	if (restoreInstead)
	{
		Msg("%s: Restoring modified Neotokyo assets...\n", szThisCaller);
	}
	else
	{
		Msg("%s: Checking for Neotokyo assets compatibility...\n", szThisCaller);
	}

	const char* filesToFix[] = {
		"materials/models/props_vehicles/rabbitfrog_engine_anim.vmt",
		"materials/models/props_vehicles/rabbitfrog_engine_anim2.vmt",
		"materials/models/props_vehicles/rabbitfrog_engine_anim3.vmt",
	};
	const char* disabledSuffix = ".DISABLED";

	CUtlString fixFrom, fixTo;
	int numSuccesses = 0;
	for (int i = 0; i < ARRAYSIZE(filesToFix); ++i)
	{
		if (!restoreInstead)
		{
			fixFrom = neoPath;
#ifdef _WIN32
			fixFrom.FixSlashes();
			fixFrom = fixFrom.Replace(":\\\\", ":\\");
#endif
			fixFrom.StripTrailingSlash();
			fixFrom.Append("/");
			fixFrom.Append(filesToFix[i]);
			fixFrom.FixSlashes();

			fixTo.Set(fixFrom);
			fixTo.Append(disabledSuffix);
		}
		else
		{
			fixTo = neoPath;
#ifdef _WIN32
			fixTo.FixSlashes();
			fixTo = fixTo.Replace(":\\\\", ":\\");
#endif
			fixTo.StripTrailingSlash();
			fixTo.Append("/");
			fixTo.Append(filesToFix[i]);
			fixTo.FixSlashes();

			fixFrom.Set(fixTo);
			fixFrom.Append(disabledSuffix);
		}

		if (filesystem->FileExists(fixFrom))
		{
#ifdef LINUX
            if (rename(fixFrom, fixTo) == 0)
#else
			if (filesystem->RenameFile(fixFrom, fixTo))
#endif
			{
				Assert(!filesystem->FileExists(fixFrom));
				Assert(filesystem->FileExists(fixTo));
				++numSuccesses;
			}
			else
			{
				Warning("%s: Rename failed: \"%s\" -> \"%s\"\n", szThisCaller, fixFrom.String(), fixTo.String());
			}
		}
		else
		{
			if (filesystem->FileExists(fixTo))
			{
				// Already handled.
				++numSuccesses;
			}
			else
			{
				Warning("%s: File doesn't exist: %s\n", szThisCaller, fixTo.String());
			}
		}
		fixFrom.Clear();
		fixTo.Clear();
	}

	const bool allFilesOk = (numSuccesses == ARRAYSIZE(filesToFix));

	if (restoreInstead)
	{
		if (allFilesOk)
		{
			Msg("%s: All Neotokyo assets restore OK.\n", szThisCaller);
		}
		else
		{
			// Something went wrong with files restore on game shutdown.
			// Error out to make sure that user sees this message.
			Error("%s: Failed to restore some temporarily modified Neotokyo assets!\n\n\
Please go to original Neotokyo game properties on Steam Library, and verify files integrity \
to ensure there aren't any corrupt files.\n\nIf you don't know why this error occurred, \
and/or think it's a bug, please report it.\n", szThisCaller);
		}
	}
	else
	{
		if (allFilesOk)
		{
			Msg("%s: All OK; scanned Neotokyo assets are compatible.\n", szThisCaller);
		}
		else
		{
			Warning("%s WARNING: Failed to temporarily rename some incompatible NT assets (is NeotokyoSource/materials write protected?);\n\
some Neotokyo assets may not load correctly!\n", szThisCaller);
		}
	}
}

// Helper override when you don't have a constructed neoPath at hand.
void FixIncompatibleNeoAssets(IFileSystem* filesystem, bool restoreInstead)
{
	const char* szThisCaller = "FixIncompatibleNeoAssets";
	char neoPath[MAX_PATH];
	bool originalNtPathOk = false;
#ifdef LINUX
	const bool callerIsClientDll = false; // always server here. should refactor this stuff later.

	// The NeotokyoSource root asset folder should exist (or be symlinked) to one of these paths,
	// or be specified with the NEO_PATH_PARM_CMD parm (which is currently broken on Linux, see below).
	// We look in the order described below, and stop looking at the first matching path.
	char neoLinuxPath_LocalSteam[MAX_PATH]{ 0 };
	char neoLinuxPath_LocalShare[MAX_PATH]{ 0 };
	const char* homePath = getenv("HOME");

	// First lookup path: user's Steam home directory. This is where Steam and SteamCMD will install by default.
	V_strcpy_safe(neoLinuxPath_LocalSteam, homePath);
	V_AppendSlash(neoLinuxPath_LocalSteam, sizeof(neoLinuxPath_LocalSteam));
	V_strcat(neoLinuxPath_LocalSteam, ".steam/steam/steamapps/common/NEOTOKYO/NeotokyoSource/", sizeof(neoLinuxPath_LocalSteam));

	// Second lookup path: user's own share directory.
	V_strcpy_safe(neoLinuxPath_LocalShare, homePath);
	V_AppendSlash(neoLinuxPath_LocalShare, sizeof(neoLinuxPath_LocalShare));
	V_strcat(neoLinuxPath_LocalShare, ".local/share/neotokyo/NeotokyoSource/", sizeof(neoLinuxPath_LocalShare));

	// Third lookup path: machine's share directory.
	const char* neoLinuxPath_UsrShare = "/usr/share/neotokyo/NeotokyoSource/";

	// NEO FIXME (Rain): getting this ParmValue from Steam Linux client seems to be broken(?),
	// we always fall back to hardcoded pDefaultVal.
	V_strcpy_safe(neoPath,
		CommandLine()->ParmValue(NEO_PATH_PARM_CMD, neoLinuxPath_LocalSteam));

	const bool isUsingCustomParm = (Q_stricmp(neoPath, neoLinuxPath_LocalSteam) != 0);

	if (isUsingCustomParm)
	{
		if (!*neoPath)
		{
			// We will crash with a more generic error later if Neo mount failed,
			// so this is our only chance to throw this more specific error message.
			Error("%s: Failed to read custom %s: '%s'\n", szThisCaller, NEO_PATH_PARM_CMD, neoPath);
		}

		if (callerIsClientDll)
		{
			DevMsg("Client using custom %s: %s\n", NEO_PATH_PARM_CMD, neoPath);
		}
		else
		{
			DevMsg("Server using custom %s: %s\n", NEO_PATH_PARM_CMD, neoPath);
		}

		originalNtPathOk = DirExists(neoPath);

		if (!originalNtPathOk)
		{
			Error("%s: Failed to access custom %s: '%s'\n", szThisCaller, NEO_PATH_PARM_CMD, neoPath);
		}
	}
	else
	{
		// Try first (default path)
		if (DirExists(neoPath))
		{
			// Do nothing; path is already set
		}
		// Try the second path
		else if (DirExists(neoLinuxPath_LocalShare))
		{
			V_strcpy_safe(neoPath, neoLinuxPath_LocalShare);
		}
		// Try the third path
		else if (DirExists(neoLinuxPath_UsrShare))
		{
			V_strcpy_safe(neoPath, neoLinuxPath_UsrShare);
		}
		// None of the paths existed
		else
		{
			Warning("%s: Could not locate original Neotokyo install!\n", szThisCaller);
		}
		
		if (!DirExists(neoPath))
		{
			Error("%s: Failed to get Neo path\n", szThisCaller);
		}
        else
        {
            originalNtPathOk = true;
        }
	}
#else
	originalNtPathOk = IsNeoGameInfoPathOK(neoPath, sizeof(neoPath));
#endif
	if (!originalNtPathOk)
	{
		Error("%s: Failed to retrieve Neo path\n", szThisCaller);
	}
	else
	{
		FixIncompatibleNeoAssets(filesystem, neoPath, restoreInstead);
	}
}

// Purpose: Find and mount files for the original Steam release of Neotokyo.
//
// On Windows, we assume to find the root "NeotokyoSource" at Steam install dir path.
// This can be overridden with NEO_PATH_PARM_CMD for supporting multiple Steam library locations.
//
// On Linux, we assume to find the root "NeotokyoSource" at hardcoded path.
// These can be installed with SteamCMD, or copied over from a Windows install.
inline bool FindOriginalNeotokyoAssets(IFileSystem *filesystem, const bool callerIsClientDll)
{
	// Server can manage without Steam, but the clients need it.
	if (callerIsClientDll && !SteamAPI_IsSteamRunning())
	{
		Error("Failed to call Steam API. This game needs to be launched through Steam.");
		return false;
	}

	if (!filesystem)
	{
		Error("Engine filesystem was not initialized properly.");
		return false;
	}

	const char *thisCaller = "FindOriginalNeotokyoAssets";
	char neoPath[MAX_PATH];
	const AppId_t neoAppId = 244630;
	const SearchPathAdd_t addType = PATH_ADD_TO_HEAD;

	bool originalNtPathOk = false;

#ifdef LINUX
	// NEO TODO (Rain): this linux path get code is repeated; should turn into a function for brevity.

	// The NeotokyoSource root asset folder should exist (or be symlinked) to one of these paths,
	// or be specified with the NEO_PATH_PARM_CMD parm (which is currently broken on Linux, see below).
    // We look in the order described below, and stop looking at the first matching path.
    char neoLinuxPath_LocalSteam[MAX_PATH] { 0 };
    char neoLinuxPath_LocalShare[MAX_PATH] { 0 };
    const char *homePath = getenv("HOME");

    // First lookup path: user's Steam home directory. This is where Steam and SteamCMD will install by default.
    V_strcpy_safe(neoLinuxPath_LocalSteam, homePath);
    V_AppendSlash(neoLinuxPath_LocalSteam, sizeof(neoLinuxPath_LocalSteam));
    V_strcat(neoLinuxPath_LocalSteam, ".steam/steam/steamapps/common/NEOTOKYO/NeotokyoSource/", sizeof(neoLinuxPath_LocalSteam));

    // Second lookup path: user's own share directory.
    V_strcpy_safe(neoLinuxPath_LocalShare, homePath);
    V_AppendSlash(neoLinuxPath_LocalShare, sizeof(neoLinuxPath_LocalShare));
    V_strcat(neoLinuxPath_LocalShare, ".local/share/neotokyo/NeotokyoSource/", sizeof(neoLinuxPath_LocalShare));

    // Third lookup path: machine's share directory.
    const char *neoLinuxPath_UsrShare = "/usr/share/neotokyo/NeotokyoSource/";
	
	// NEO FIXME (Rain): getting this ParmValue from Steam Linux client seems to be broken(?),
	// we always fall back to hardcoded pDefaultVal.
	V_strcpy_safe(neoPath,
        CommandLine()->ParmValue(NEO_PATH_PARM_CMD, neoLinuxPath_LocalSteam));

    const bool isUsingCustomParm = (Q_stricmp(neoPath, neoLinuxPath_LocalSteam) != 0);

	if (isUsingCustomParm)
	{
		if (!*neoPath)
		{
			// We will crash with a more generic error later if Neo mount failed,
			// so this is our only chance to throw this more specific error message.
			Error("%s: Failed to read custom %s: '%s'\n", thisCaller, NEO_PATH_PARM_CMD, neoPath);
		}

		if (callerIsClientDll)
		{
			DevMsg("Client using custom %s: %s\n", NEO_PATH_PARM_CMD, neoPath);
		}
		else
		{
			DevMsg("Server using custom %s: %s\n", NEO_PATH_PARM_CMD, neoPath);
		}

        originalNtPathOk = DirExists(neoPath);

		if (!originalNtPathOk)
		{
			Error("%s: Failed to access custom %s: '%s'\n", thisCaller, NEO_PATH_PARM_CMD, neoPath);
		}
	}
	else
    {
        // Try first (default path)
        if (DirExists(neoPath))
        {
            // Do nothing; path is already set
        }
        // Try the second path
        else if (DirExists(neoLinuxPath_LocalShare))
        {
            V_strcpy_safe(neoPath, neoLinuxPath_LocalShare);
        }
        // Try the third path
        else if (DirExists(neoLinuxPath_UsrShare))
        {
            V_strcpy_safe(neoPath, neoLinuxPath_UsrShare);
        }
        // None of the paths existed
        else
        {
            Warning("%s: Could not locate original Neotokyo install!\n", thisCaller);
        }

        originalNtPathOk = DirExists(neoPath);
	}

	// Both client & server call this function; only print the informational stuff once.
	if (callerIsClientDll)
	{
		DevMsg("%s: Linux build; searching for Neo mount from path: '%s'\n", thisCaller, neoPath);
	}

#else // If Windows
	const char *noNeoPathId = "0";
	Q_strncpy(neoPath, CommandLine()->ParmValue(NEO_PATH_PARM_CMD, noNeoPathId), sizeof(neoPath));

	// There was no NEO_PATH_PARM_CMD provided
	if (!*neoPath || FStrEq(neoPath, noNeoPathId))
	{
		// User has Steam running, use it to deduce the NT path.
		if (SteamAPI_IsSteamRunning())
		{
			originalNtPathOk = IsNeoGameInfoPathOK(neoPath, sizeof(neoPath));
		}
		else
		{
			// We don't have Steam running, and there is no NEO_PATH_PARM_CMD specified.
			// This is a failure state on Windows.
			originalNtPathOk = false;
		}
	}
	/// There is a NEO_PATH_PARM_CMD
	else
	{
		// Make sure the path is valid
		if (filesystem->IsDirectory(neoPath))
		{
			// Make sure there's actually a GameInfo.txt in the path, otherwise we will crash on mount.
			// We make the assumption that any GameInfo.txt found will be valid format.
			char gameInfoPath[MAX_PATH];
			V_strcpy_safe(gameInfoPath, neoPath);
			V_AppendSlash(gameInfoPath, sizeof(gameInfoPath));
			V_strcat(gameInfoPath, "GameInfo.txt", sizeof(gameInfoPath));

			originalNtPathOk = filesystem->FileExists(gameInfoPath);
		}

		// NEO TODO (Rain): check reg entries:
		//	HKEY_LOCAL_MACHINE\SOFTWARE\Valve\Steam (32bit) and HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Valve\Steam (64bit)
		// for a Steam installation as fallback on client systems.
	}
	
#endif

	if (originalNtPathOk)
	{
#ifdef LINUX
		if (strlen(neoPath) > 0)
		{
			V_AppendSlash(neoPath, sizeof(neoPath));

			filesystem->AddSearchPath(neoPath, NEO_MOUNT_PATHID, addType);

			if (callerIsClientDll)
			{
				DevMsg("%s: Added '%s' to path.\n", thisCaller, neoPath);
			}

			FilesystemMountRetval_t mountStatus =
				filesystem->MountSteamContent(-neoAppId);
			if (mountStatus == FILESYSTEM_MOUNT_OK)
			{
				if (callerIsClientDll)
				{
					DevMsg("Neotokyo AppID (%i) mount OK.\n", neoAppId);
				}
			}
			else
			{
				Warning("%s: Failed to mount Neotokyo AppID (%i)\n", thisCaller, neoAppId);
				return false;
			}
		}
		else
		{
			Warning("%s: Failed to parse original Neotokyo files location\n", thisCaller);
			return false;
		}
#else // If Windows
		filesystem->AddSearchPath(neoPath, NEO_MOUNT_PATHID, addType);

		if (callerIsClientDll)
		{
			DevMsg("%s: Added '%s' to path.\n", thisCaller, neoPath);
		}

		FilesystemMountRetval_t mountStatus =
			filesystem->MountSteamContent(-(int)neoAppId);
		if (mountStatus == FILESYSTEM_MOUNT_OK)
		{
			if (callerIsClientDll)
			{
				DevMsg("Neotokyo AppID (%i) mount OK.\n", neoAppId);
			}
		}
		else
		{
			Warning("%s: Failed to mount Neotokyo AppID (%i)\n", thisCaller, neoAppId);
			return false;
		}
#endif
		// Called by both client and server, but we only need to fix these once.
		// Call serverside so dedicated servers get these, too.
		if (!callerIsClientDll)
		{
#ifdef LINUX
			FixCaseSensitivePathsForLinux(neoPath);
#endif
			FixIncompatibleNeoAssets(filesystem, neoPath, false);
		}
	}
	else // originalNtPathOk
	{
#ifdef LINUX
        const char *steamcmdWinDlOption = "./steamcmd.sh +@sSteamCmdForcePlatformType windows";
        Error("%s: Original Neotokyo installation was not found. \
Please use SteamCMD to download the Neotokyo (Windows) contents to one of these paths:\
\n\n    1. '%s', or\n    2. '%s', or\n    3. '%s'\n\nThe paths are checked in this listed \
order, using the first match.\n\nTip: To enable Windows file downloads on Linux SteamCMD, \
you may invoke it as: \"%s\"\n\nExample SteamCMD input:\n\n    // Your Steam log in name \
(log in to Steam client beforehand to prepare a login cookie and avoid authentication)\n\
    login your-username\n    // Install NT client using its AppID\n    app_update %d \
validate\n    quit\n",
            thisCaller, neoLinuxPath_LocalSteam, neoLinuxPath_LocalShare,
            neoLinuxPath_UsrShare, steamcmdWinDlOption, neoAppId);
#else
		Error("%s: Original Neotokyo installation was not found (looked at path: '%s'). \
Please install Neotokyo on Steam for this mod to work.\n\n\
A) If you are launching Neotokyo as a game, make sure you are running Steam while launching.\n\
If you are seeing this error while Steam is running, this is probably a bug. You may try the \
SRCDS fix described below, but please report the issue.\n\n\
B) If you are running a Steamless SRCDS server instance, use the \"%s\" launch argument \
to specify your NeotokyoSource root folder install location.\n\
Example: %s \"C:\\\\srcds\\NEOTOKYO\\NeotokyoSource\"",
	thisCaller,
	(FStrEq(neoPath, noNeoPathId) ? "(no path)" : neoPath),
	NEO_PATH_PARM_CMD,
	NEO_PATH_PARM_CMD);
#endif
		return false;
	}

	return true;
}

#endif // NEO_MOUNT_ORIGINAL_H
