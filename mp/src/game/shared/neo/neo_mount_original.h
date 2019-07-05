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
			if (callerIsClientDll)
			{
				DevMsg("Client using custom -neopath: %s\n", neoPath);
			}
			else
			{
				DevMsg("Server using custom -neopath: %s\n", neoPath);
			}
		}
	}

	// Both client & server call this function; only print the informational stuff once.
	if (callerIsClientDll)
	{
		DevMsg("%s: Linux build; expecting to find original Neotokyo assets at: '%s'\n", thisCaller, neoPath);
	}

	StatStruct file_stat;
	originalNtPathOk = ( Stat(neoPath, &file_stat) == 0
		&& IsDir(file_stat) );
#else // If Windows

	char neoWindowsDefaultPath[MAX_PATH];

	if (SteamAPI_IsSteamRunning())
	{
		// Client falls back to Steam's default Source mod install path, if -neopath was not specified.
		Q_strncpy(neoWindowsDefaultPath, SteamAPI_GetSteamInstallPath(), sizeof(neoWindowsDefaultPath));
		V_AppendSlash(neoWindowsDefaultPath, sizeof(neoWindowsDefaultPath));
		V_strcat(neoWindowsDefaultPath, "steamapps\\common\\NEOTOKYO\\NeotokyoSource", sizeof(neoWindowsDefaultPath));
	}
	else
	{
		// NEO TODO (Rain): check reg entries:
		//	HKEY_LOCAL_MACHINE\SOFTWARE\Valve\Steam (32bit) and HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Valve\Steam (64bit)
		// for a Steam installation as fallback on client systems.

		// A generic, plausible Windows server srcds location for fallback, if -neopath was not specified.
		const char serverWinDefault[MAX_PATH] = "C:\\srcds\\NeotokyoSource";
		Q_strncpy(neoWindowsDefaultPath, serverWinDefault, sizeof(serverWinDefault));
	}

	Q_strncpy(neoPath, CommandLine()->ParmValue("-neopath", neoWindowsDefaultPath), sizeof(neoPath));

	originalNtPathOk = filesystem->IsDirectory(neoPath);
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
Please use SteamCMD to download the Neotokyo (Windows) contents to path: '%s'\n",
	thisCaller, neoHardcodedLinuxAssetPath);
#else
		// NEO TODO (Rain): we can attempt to recover here clientside by parsing SteamApps\libraryfolders.vdf
		// for additional Steam drives where Neotokyo might be located.

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