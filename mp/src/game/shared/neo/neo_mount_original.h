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

// Used to determine whether directories exist.
#ifdef LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef LINUX
typedef struct stat StatStruct;
static inline int Stat(const char* path, StatStruct* buf) { return stat(path, buf); }
static inline bool IsDir(const StatStruct& st) { return S_ISDIR(st.st_mode); }
#else
// Attempts to build a valid NT game path using the provided Neo path, and Steam info.
// Returns true/false depending on if we successfully found a valid path.
// NOTE: this *will* modify the input path pointer, regardless of success!!
static inline bool IsNeoGameInfoPathOK(char *out_neoPath, const int pathLen)
{
	// This only works for Steam users, and should only be called for them.
	if (!SteamAPI_IsSteamRunning())
	{
		Assert(false);
		return false;
	}

	// Get Steam path
	char pathBuffer[MAX_PATH];
	V_strcpy_safe(pathBuffer, SteamAPI_GetSteamInstallPath());
	V_AppendSlash(pathBuffer, sizeof(pathBuffer));

	// Try and see if we have NT installed under the base Steam path
	V_strcat(pathBuffer, "steamapps\\common\\NEOTOKYO\\NeotokyoSource", sizeof(pathBuffer));
	if (filesystem->IsDirectory(pathBuffer))
	{
		V_AppendSlash(pathBuffer, sizeof(pathBuffer));
		V_strcat(pathBuffer, "GameInfo.txt", sizeof(pathBuffer));
		if (filesystem->FileExists(pathBuffer))
		{
			V_strncpy(out_neoPath, pathBuffer, pathLen);
			return true;
		}
	}

	// Otherwise, look up additional Steam library folders elsewhere
	V_strcpy_safe(pathBuffer, SteamAPI_GetSteamInstallPath());
	V_AppendSlash(pathBuffer, sizeof(pathBuffer));
	V_strcat(pathBuffer, "steamapps\\libraryfolders.vdf", sizeof(pathBuffer));

	bool bGameInfoPathIsOK = false;
	KeyValues *pKvLibFolders = new KeyValues("LibraryFolders");
	if (pKvLibFolders->LoadFromFile(filesystem, pathBuffer))
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

// Purpose: Find and mount files for the original Steam release of Neotokyo.
//
// On Windows, we assume to find the root "NeotokyoSource" at Steam install dir path.
// This can be overridden with -neopath for supporting multiple Steam library locations.
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
	const char *pathID = "GAME";
	const SearchPathAdd_t addType = PATH_ADD_TO_HEAD;

	bool originalNtPathOk = false;

#ifdef LINUX
	// The NeotokyoSource root asset folder should exist (or be symlinked) to one of these paths,
	// or be specified with -neopath parm (which is currently broken on Linux, see below).
	// We stop looking on first folder that exists.
	char neoHardcodedLinuxAssetPath_Home[MAX_PATH];
	V_strcpy_safe(neoHardcodedLinuxAssetPath_Home, getenv("HOME"));
	V_AppendSlash(neoHardcodedLinuxAssetPath_Home, sizeof(neoHardcodedLinuxAssetPath_Home));
	V_strcat(neoHardcodedLinuxAssetPath_Home, ".local/share/neotokyo/NeotokyoSource/",
		sizeof(neoHardcodedLinuxAssetPath_Home));

	const char *neoHardcodedLinuxAssetPath_Share = "/usr/share/neotokyo/NeotokyoSource/";
	
	// NEO FIXME (Rain): getting this ParmValue from Steam Linux client seems to be broken(?),
	// we always fall back to hardcoded pDefaultVal.
	V_strcpy_safe(neoPath,
		CommandLine()->ParmValue("-neopath", neoHardcodedLinuxAssetPath_Home));

	const bool isUsingCustomParm = (Q_stricmp(neoPath, neoHardcodedLinuxAssetPath_Home) != 0);

	StatStruct file_stat;

	if (isUsingCustomParm)
	{
		if (!*neoPath)
		{
			// We will crash with a more generic error later if Neo mount failed,
			// so this is our only chance to throw this more specific error message.
			Error("%s: Failed to read custom -neopath: '%s'\n", thisCaller, neoPath);
		}

		if (callerIsClientDll)
		{
			DevMsg("Client using custom -neopath: %s\n", neoPath);
		}
		else
		{
			DevMsg("Server using custom -neopath: %s\n", neoPath);
		}

		originalNtPathOk = ( Stat(neoPath, &file_stat) == 0 && IsDir(file_stat) );

		if (!originalNtPathOk)
		{
			Error("%s: Failed to access custom -neopath: '%s'\n", thisCaller, neoPath);
		}
	}
	else
	{
		// Try first path
		originalNtPathOk = ( Stat(neoPath, &file_stat) == 0 && IsDir(file_stat) );

		// Try second path
		if (!originalNtPathOk)
		{
			V_strcpy_safe(neoPath, neoHardcodedLinuxAssetPath_Share);
			
			originalNtPathOk = ( Stat(neoPath, &file_stat) == 0 && IsDir(file_stat) );
		}
	}

	// Both client & server call this function; only print the informational stuff once.
	if (callerIsClientDll)
	{
		DevMsg("%s: Linux build; searching for Neo mount from path: '%s'\n", thisCaller, neoPath);
	}

#else // If Windows
	const char *noNeoPathId = "0";
	Q_strncpy(neoPath, CommandLine()->ParmValue("-neopath", noNeoPathId), sizeof(neoPath));

	// There was no -neopath provided
	if (!*neoPath || FStrEq(neoPath, noNeoPathId))
	{
		// User has Steam running, use it to deduce the NT path.
		if (SteamAPI_IsSteamRunning())
		{
			originalNtPathOk = IsNeoGameInfoPathOK(neoPath, sizeof(neoPath));
		}
		else
		{
			// We don't have Steam running, and there is no -neopath specified.
			// This is a failure state on Windows.
			originalNtPathOk = false;
		}
	}
	/// There is a -neopath
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

			filesystem->AddSearchPath(neoPath, pathID, addType);

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
					Msg("Neotokyo AppID (%i) mount OK.\n", neoAppId);
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
		filesystem->AddSearchPath(neoPath, pathID, addType);

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
				Msg("Neotokyo AppID (%i) mount OK.\n", neoAppId);
			}
		}
		else
		{
			Warning("%s: Failed to mount Neotokyo AppID (%i)\n", thisCaller, neoAppId);
			return false;
		}
#endif
	}
	else // originalNtPathOk
	{
#ifdef LINUX
		Error("%s: Original Neotokyo installation was not found. \
Please use SteamCMD to download the Neotokyo (Windows) contents to one of these paths:\n\n'%s',\n'%s'\n",
	thisCaller, neoHardcodedLinuxAssetPath_Home, neoHardcodedLinuxAssetPath_Share);
#else
		Error("%s: Original Neotokyo installation was not found (looked at path: '%s'). \
Please install Neotokyo on Steam for this mod to work.\n\n\
If your original Neotokyo path differs from Steam install path (or if you are running a Steamless \
dedicated server instance), use the -neopath launch argument to specify your NeotokyoSource root \
folder install location.", thisCaller, neoPath);
#endif
		return false;
	}

	return true;
}

#endif // NEO_MOUNT_ORIGINAL_H