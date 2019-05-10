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
// On Windows, this should work if user has Neotokyo installed on Steam.
//
// On Linux, we assume to find the root "NeotokyoSource" at hardcoded path.
// These can be installed with SteamCMD, or copied over from a Windows install.
inline bool FindOriginalNeotokyoAssets(IFileSystem *g_pFullFileSystem)
{
    if (!g_pFullFileSystem)
    {
        return false;
    }

    const char *thisCaller = "FindOriginalNeotokyoAssets";
	char neoPath[MAX_PATH * 2];
	const AppId_t neoAppId = 47182;
	const char *pathID = "GAME";
	const SearchPathAdd_t addType = PATH_ADD_TO_HEAD;

	ISteamApps *apps = SteamClient()->GetISteamApps(
		GetHSteamUser(), GetHSteamPipe(), STEAMAPPS_INTERFACE_VERSION);

	if (!apps)
	{
		return false;
	}

	bool originalNtPathOk = false;
#ifdef LINUX
	// The NeotokyoSource root asset folder should exist (or be symlinked) here,
	// or in the custom neopath.
	const char *neoHardcodedLinuxAssetPath = "/usr/share/neotokyo/NeotokyoSource/";

	// NEO FIXME (Rain): getting this ParmValue from Steam Linux client seems to be broken(?),
	// we always fall back to hardcoded pDefaultVal.
	Q_strncpy(neoPath,
		CommandLine()->ParmValue ("-neopath", neoHardcodedLinuxAssetPath),
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
    if (developer.GetBool())
    {
        Msg("%s: Linux build; expecting to find original Neotokyo assets at: '%s'\n", thisCaller, neoPath);
    }
#endif

	StatStruct file_stat;
	originalNtPathOk = ( Stat(neoPath, &file_stat) == 0
		&& IsDir(file_stat) );
#else // If Windows
	originalNtPathOk = apps->BIsAppInstalled(neoAppId);
#endif

	if (originalNtPathOk)
	{
#ifdef LINUX
		if (strlen(neoPath) > 0)
		{
			V_AppendSlash(neoPath, sizeof(neoPath));
			g_pFullFileSystem->AddSearchPath(neoPath, pathID, addType);

#ifdef CLIENT_DLL
            if (developer.GetBool())
            {
                Msg("%s: Added '%s' to path.\n", thisCaller, neoPath);
            }
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
		int pathLen = apps->GetAppInstallDir(neoAppId, neoPath, sizeof(neoPath));
		if (pathLen > 0)
		{
			V_AppendSlash(neoPath, sizeof(neoPath));
			V_strncat(neoPath, "NeotokyoSource", sizeof(neoPath));
			g_pFullFileSystem->AddSearchPath(neoPath, pathID, addType);
#ifdef CLIENT_DLL
            if (developer.GetBool())
            {
                Msg("%s: Added '%s' to path.\n", thisCaller, neoPath);
            }
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
#endif
	}
	else // originalNtPathOk
	{
#ifdef LINUX
		Warning("%s: Original Neotokyo installation was not found. \
Please use SteamCMD to download the Neotokyo (Windows) contents to path: '%s'\n", thisCaller, neoHardcodedLinuxAssetPath);
#else
		Warning("%s: Original Neotokyo installation was not found. \
Please install Neotokyo on Steam for this mod to work.\n", thisCaller);
#endif
		return false;
	}
	return true;
}

#endif // NEO_MOUNT_ORIGINAL_H