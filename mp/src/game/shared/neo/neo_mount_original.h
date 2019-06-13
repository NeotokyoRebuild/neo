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
#endif

// Purpose: Find and mount files for the original Steam release of Neotokyo.
//
// On Windows, we assume to find the root "NeotokyoSource" at Steam install dir path.
// This can be overridden with -neopath for supporting multiple Steam library locations.
//
// On Linux, we assume to find the root "NeotokyoSource" at hardcoded path.
// These can be installed with SteamCMD, or copied over from a Windows install.
inline bool FindOriginalNeotokyoAssets(IFileSystem *g_pFullFileSystem)
{
	// We can't mount Neotokyo if these fail. Crash with an error message.
	if (!SteamAPI_IsSteamRunning())
	{
		Error("Failed to call Steam API. This game needs to be launched through Steam.");
		return false;
	}
	else if (!g_pFullFileSystem)
	{
		Error("Engine filesystem was not initialized properly.");
		return false;
	}

	const char *thisCaller = "FindOriginalNeotokyoAssets";
	char neoPath[MAX_PATH];
	const AppId_t neoAppId = 47182;
	const char *pathID = "GAME";
	const SearchPathAdd_t addType = PATH_ADD_TO_HEAD;

	bool originalNtPathOk = false;
#ifdef LINUX
	// The NeotokyoSource root asset folder should exist (or be symlinked) here,
	// or in the custom neopath.
	const char *neoHardcodedLinuxAssetPath = "/usr/share/neotokyo/NeotokyoSource/";

	// NEO FIXME (Rain): getting this ParmValue from Steam Linux client seems to be broken(?),
	// we always fall back to hardcoded pDefaultVal.
	Q_strncpy(neoPath,
		CommandLine()->ParmValue("-neopath", neoHardcodedLinuxAssetPath),
		sizeof(neoPath));
	
	if (Q_stricmp(neoPath, neoHardcodedLinuxAssetPath) != 0)
	{
		if (!*neoPath)
		{
			strcpy(neoPath, neoHardcodedLinuxAssetPath);
			Warning("Failed to parse custom -neopath, reverting to: %s\n",
				neoHardcodedLinuxAssetPath);
		}
		else
		{
#ifdef CLIENT_DLL
			Msg("Client using custom -neopath: %s\n", neoPath);
#else
			Msg("Server using custom -neopath: %s\n", neoPath);
#endif
		}
	}

#ifdef CLIENT_DLL // Both client & server call this function; only print the informational stuff once.
	DevMsg("%s: Linux build; expecting to find original Neotokyo assets at: '%s'\n", thisCaller, neoPath);
#endif

	StatStruct file_stat;
	originalNtPathOk = ( Stat(neoPath, &file_stat) == 0
		&& IsDir(file_stat) );
#else // If Windows
	char neoWindowsDefaultPath[MAX_PATH];
	Q_strncpy(neoWindowsDefaultPath, SteamAPI_GetSteamInstallPath(), sizeof(neoWindowsDefaultPath));
	V_AppendSlash(neoWindowsDefaultPath, sizeof(neoWindowsDefaultPath));
	V_strcat(neoWindowsDefaultPath, "steamapps\\common\\NEOTOKYO\\NeotokyoSource", sizeof(neoWindowsDefaultPath));

	Q_strncpy(neoPath, CommandLine()->ParmValue("-neopath", neoWindowsDefaultPath), sizeof(neoPath));

	originalNtPathOk = g_pFullFileSystem->IsDirectory(neoPath);
#endif

	if (originalNtPathOk)
	{
#ifdef LINUX
		if (strlen(neoPath) > 0)
		{
			V_AppendSlash(neoPath, sizeof(neoPath));
			g_pFullFileSystem->AddSearchPath(neoPath, pathID, addType);

#ifdef CLIENT_DLL
			DevMsg("%s: Added '%s' to path.\n", thisCaller, neoPath);
#endif
			
			FilesystemMountRetval_t mountStatus =
				g_pFullFileSystem->MountSteamContent(-neoAppId);
			if (mountStatus == FILESYSTEM_MOUNT_OK)
			{
#ifdef CLIENT_DLL
				Msg("Neotokyo AppID (%i) mount OK.\n", neoAppId);
#endif
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
		g_pFullFileSystem->AddSearchPath(neoPath, pathID, addType);

#ifdef CLIENT_DLL
		DevMsg("%s: Added '%s' to path.\n", thisCaller, neoPath);
#endif
		
		FilesystemMountRetval_t mountStatus =
			g_pFullFileSystem->MountSteamContent(-(int)neoAppId);
		if (mountStatus == FILESYSTEM_MOUNT_OK)
		{
#ifdef CLIENT_DLL
			Msg("Neotokyo AppID (%i) mount OK.\n", neoAppId);
#endif
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
		Warning("%s: Original Neotokyo installation was not found. \
Please use SteamCMD to download the Neotokyo (Windows) contents to path: '%s'\n",
	thisCaller, neoHardcodedLinuxAssetPath);
#else
		Warning("%s: Original Neotokyo installation was not found (looked at path: '%s'). \
Please install Neotokyo on Steam for this mod to work. If your Neotokyo path \
differs from Steam install path, use the -neopath launch argument to specify your \
NeotokyoSource root folder install location.\n", thisCaller, neoPath);
#endif
		return false;
	}

	return true;
}

#endif // NEO_MOUNT_ORIGINAL_H