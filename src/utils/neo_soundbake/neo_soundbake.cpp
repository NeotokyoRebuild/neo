//========= NEOTOKYO;REBUILD =========//
//
// neo_soundbake: bakes a map's acoustic probes (Steam Audio reverb and pathing) at compile time
// and stores them in the BSP as a game lump, so the game loads them instead of baking at runtime.
//
// Run after VRAD, on the BSP it wrote:
//     neo_soundbake -game <mod dir> [-threads <n>] [-phonon <library>] [-appid_dir_<appid> <dir>] <map.bsp>
// In Hammer++'s expert compile configuration: $bindir\neo_soundbake.exe, parameters
//     -game $gamedir $path\$file.bsp
// after the VRAD step and before the copy into the game's maps directory.
//
// It places probes and builds the acoustic scene with the very code the client uses
// (game/client/neo/audio), against the same content: the engine file system with the mod's
// gameinfo.txt search paths, the map's own pakfile, and vphysics for surface properties and static
// prop collision. It must therefore live in the engine's bin/<platform> directory (bin/x64 or
// bin/linux64), next to filesystem_stdio and vphysics, like VBSP and VRAD. Windows and Linux.
//
//====================================//

#include <stdint.h>
#include <stdio.h>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <direct.h>
#else
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "bspfile.h"
#include "filesystem.h"
#include "filesystem_init.h"
#include "KeyValues.h"
#include "tier0/dbg.h"
#include "tier0/icommandline.h"
#include "tier0/platform.h"
#include "tier0/threadtools.h"
#include "tier1/interface.h"
#include "tier1/strtools.h"
#include "tier1/utlbuffer.h"
#include "vphysics_interface.h"

#include "neo_audio_geometry.h"
#include "neo_audio_probe_lump.h"
#include "neo_audio_probes.h"
#include "neo_spatializer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// utils/lzma/lzma.cpp: Valve's lzma_header_t format, which the client's CLZMA decodes. Free with free().
unsigned char *LZMA_Compress(unsigned char *pInput, unsigned int inputSize, unsigned int *pOutputSize);

namespace
{

// As the client renders; the bake itself does not depend on them.
constexpr int kBakeSampleRate = 48000;
constexpr int kBakeFrameSize = 512;
constexpr float kBakeProgressStep = 0.05f;
constexpr unsigned kBakePollMs = 250;

#ifdef _WIN32
constexpr char kPhononLibrary[] = "phonon.dll";
#else
constexpr char kPhononLibrary[] = "libphonon.so";

// Where Steam keeps steamapps on Linux, relative to $HOME: the native client's links and data
// directory, and the Flatpak's.
const char *const kSteamRootsUnderHome[] = {
	".steam/steam",
	".steam/root",
	".local/share/Steam",
	".var/app/com.valvesoftware.Steam/.local/share/Steam",
};
#endif

constexpr char kGameInfoPathToken[] = "|gameinfo_path|";
constexpr char kAppIdToken[] = "|appid_";
constexpr char kSurfacePropManifest[] = "scripts/surfaceproperties_manifest.txt";

IFileSystem *g_pBakeFileSystem = nullptr;

bool Fail(const char *pszFormat, ...)
{
	char message[2048];
	va_list args;
	va_start(args, pszFormat);
	V_vsnprintf(message, sizeof(message), pszFormat, args);
	va_end(args);
	Warning("neo_soundbake: %s\n", message);
	return false;
}

void ToForwardSlashes(char *pszPath)
{
	V_FixSlashes(pszPath, '/');
}

//-----------------------------------------------------------------------------
// Appid mounting. gameinfo.txt mounts Source SDK Base content with |appid_N|, which the SDK's
// tool file system init refuses ("not supported on non-engine DLL projects"). So the install
// directory is found the way Steam records it, and the paths are rewritten before loading.
//-----------------------------------------------------------------------------
bool LoadKeyValuesAbsolute(KeyValues *pKv, const char *pszPath)
{
	pKv->UsesEscapeSequences(true);
	return pKv->LoadFromFile(g_pBakeFileSystem, pszPath, nullptr);
}

bool FindSteamRoot(char *pszOut, int outSize)
{
#ifdef _WIN32
	DWORD size = static_cast<DWORD>(outSize);
	if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", RRF_RT_REG_SZ, nullptr, pszOut, &size) == ERROR_SUCCESS)
	{
		ToForwardSlashes(pszOut);
		return true;
	}
#else
	// The first install that has a library list; the native client's links usually resolve to
	// the same place.
	const char *pszHome = getenv("HOME");
	for (const char *pszRoot : kSteamRootsUnderHome)
	{
		char libraries[MAX_PATH];
		V_snprintf(pszOut, outSize, "%s/%s", pszHome ? pszHome : "", pszRoot);
		V_snprintf(libraries, sizeof(libraries), "%s/steamapps/libraryfolders.vdf", pszOut);
		if (pszHome && access(libraries, R_OK) == 0)
		{
			return true;
		}
	}
#endif
	return false;
}

// A private directory for the resolved gameinfo.txt; removed again by RemoveTempDir.
void MakeTempDir(char *pszOut, int outSize)
{
#ifdef _WIN32
	char tempRoot[MAX_PATH];
	GetTempPathA(sizeof(tempRoot), tempRoot);
	V_snprintf(pszOut, outSize, "%sneo_soundbake_%lu", tempRoot, GetCurrentProcessId());
	_mkdir(pszOut);
#else
	const char *pszTempRoot = getenv("TMPDIR");
	V_snprintf(pszOut, outSize, "%s/neo_soundbake_%d", (pszTempRoot && *pszTempRoot) ? pszTempRoot : "/tmp",
			   static_cast<int>(getpid()));
	if (mkdir(pszOut, 0700) != 0 && errno != EEXIST)
	{
		Warning("neo_soundbake: cannot create %s: %s\n", pszOut, strerror(errno));
	}
#endif
}

void RemoveTempDir(const char *pszDir)
{
#ifdef _WIN32
	_rmdir(pszDir);
#else
	rmdir(pszDir);
#endif
}

bool ResolveAppInstallDir(int appId, char *pszOut, int outSize)
{
	// An explicit -appid_dir_<appid> <dir> wins, e.g. for a Steam install in an unusual place.
	char overrideParm[64];
	V_snprintf(overrideParm, sizeof(overrideParm), "-appid_dir_%d", appId);
	if (const char *pszOverride = CommandLine()->ParmValue(overrideParm))
	{
		V_strncpy(pszOut, pszOverride, outSize);
		ToForwardSlashes(pszOut);
		return true;
	}

	char steamRoot[MAX_PATH];
	if (!FindSteamRoot(steamRoot, sizeof(steamRoot)))
	{
		return Fail("cannot find Steam to resolve appid %d; pass %s <dir>", appId, overrideParm);
	}

	char path[MAX_PATH];
	V_snprintf(path, sizeof(path), "%s/steamapps/libraryfolders.vdf", steamRoot);
	KeyValues::AutoDelete pLibraries("libraryfolders");
	if (!LoadKeyValuesAbsolute(pLibraries, path))
	{
		return Fail("cannot read %s to resolve appid %d; pass %s <dir>", path, appId, overrideParm);
	}

	char appKey[32];
	V_snprintf(appKey, sizeof(appKey), "%d", appId);
	for (KeyValues *pLibrary = pLibraries->GetFirstTrueSubKey(); pLibrary; pLibrary = pLibrary->GetNextTrueSubKey())
	{
		KeyValues *pApps = pLibrary->FindKey("apps");
		const char *pszLibraryPath = pLibrary->GetString("path");
		if (!pApps || !pApps->FindKey(appKey) || !*pszLibraryPath)
		{
			continue;
		}

		V_snprintf(path, sizeof(path), "%s/steamapps/appmanifest_%d.acf", pszLibraryPath, appId);
		KeyValues::AutoDelete pManifest("AppState");
		if (!LoadKeyValuesAbsolute(pManifest, path) || !*pManifest->GetString("installdir"))
		{
			return Fail("cannot read %s; pass %s <dir>", path, overrideParm);
		}
		V_snprintf(pszOut, outSize, "%s/steamapps/common/%s", pszLibraryPath, pManifest->GetString("installdir"));
		ToForwardSlashes(pszOut);
		return true;
	}
	return Fail("appid %d is not installed in any Steam library; install it or pass %s <dir>", appId, overrideParm);
}

// Writes a copy of gameInfoDir/gameinfo.txt into tempDir with |gameinfo_path| and |appid_N|
// replaced by absolute directories, which the SDK search path loader handles natively.
bool WriteResolvedGameInfo(const char *pszGameInfoDir, const char *pszTempDir)
{
	char path[MAX_PATH];
	V_snprintf(path, sizeof(path), "%s/gameinfo.txt", pszGameInfoDir);
	KeyValues::AutoDelete pGameInfo("GameInfo");
	if (!pGameInfo->LoadFromFile(g_pBakeFileSystem, path, nullptr))
	{
		return Fail("cannot read %s", path);
	}
	KeyValues *pFileSystem = pGameInfo->FindKey("FileSystem");
	KeyValues *pSearchPaths = pFileSystem ? pFileSystem->FindKey("SearchPaths") : nullptr;
	if (!pSearchPaths)
	{
		return Fail("%s has no FileSystem/SearchPaths", path);
	}

	char gameInfoDir[MAX_PATH];
	V_strncpy(gameInfoDir, pszGameInfoDir, sizeof(gameInfoDir));
	ToForwardSlashes(gameInfoDir);
	for (KeyValues *pPath = pSearchPaths->GetFirstValue(); pPath; pPath = pPath->GetNextValue())
	{
		const char *pszLocation = pPath->GetString();
		char resolved[MAX_PATH];
		if (V_strnicmp(pszLocation, kGameInfoPathToken, sizeof(kGameInfoPathToken) - 1) == 0)
		{
			V_snprintf(resolved, sizeof(resolved), "%s/%s", gameInfoDir, pszLocation + sizeof(kGameInfoPathToken) - 1);
		}
		else if (V_strnicmp(pszLocation, kAppIdToken, sizeof(kAppIdToken) - 1) == 0)
		{
			const char *pszAppId = pszLocation + sizeof(kAppIdToken) - 1;
			const char *pszRest = strchr(pszAppId, '|');
			char installDir[MAX_PATH];
			if (!pszRest || !ResolveAppInstallDir(atoi(pszAppId), installDir, sizeof(installDir)))
			{
				return pszRest ? false : Fail("malformed search path %s in %s", pszLocation, path);
			}
			V_snprintf(resolved, sizeof(resolved), "%s/%s", installDir, pszRest + 1);
		}
		else
		{
			continue;
		}
		ToForwardSlashes(resolved);
		pPath->SetStringValue(resolved);
	}

	// Plain stdio: the temp directory is outside the file system's write paths, which it complains about.
	CUtlBuffer text(0, 0, CUtlBuffer::TEXT_BUFFER);
	pGameInfo->RecursiveSaveToFile(text, 0);
	V_snprintf(path, sizeof(path), "%s/gameinfo.txt", pszTempDir);
	FILE *pFile = fopen(path, "wb");
	const bool bWritten = pFile && fwrite(text.Base(), 1, text.TellPut(), pFile) == static_cast<size_t>(text.TellPut());
	if (pFile)
	{
		fclose(pFile);
	}
	return bWritten ? true : Fail("cannot write %s", path);
}

bool InitFileSystem(const char *pszBspPath, char *pszGameInfoDir, int gameInfoDirSize)
{
	// The SDK falls back to a file system that no longer exists when filesystem_stdio is not beside
	// the executable, with an unhelpful message; say where the tool has to be instead.
	char fileSystemDll[MAX_PATH];
	bool bSteam = false;
	if (FileSystem_GetFileSystemDLLName(fileSystemDll, sizeof(fileSystemDll), bSteam) != FS_OK || bSteam
		|| !V_stristr(fileSystemDll, "filesystem_stdio"))
	{
		return Fail("filesystem_stdio is not next to neo_soundbake: put it in the engine's bin%s directory, beside VBSP",
					PLATFORM_DIR);
	}

	CFSLoadModuleInfo loadInfo;
	loadInfo.m_pFileSystemDLLName = fileSystemDll;
	loadInfo.m_pDirectoryName = pszBspPath;
	loadInfo.m_bOnlyUseDirectoryName = false;
	loadInfo.m_ConnectFactory = Sys_GetFactoryThis();
	loadInfo.m_bSteam = bSteam;
	loadInfo.m_bToolsMode = true;
	if (FileSystem_LoadFileSystemModule(loadInfo) != FS_OK)
	{
		return Fail("cannot load the file system (is the tool next to filesystem_stdio, and -game set?)");
	}
	g_pBakeFileSystem = loadInfo.m_pFileSystem;

	CFSMountContentInfo mountInfo;
	mountInfo.m_pDirectoryName = loadInfo.m_GameInfoPath;
	mountInfo.m_pFileSystem = loadInfo.m_pFileSystem;
	mountInfo.m_bToolsMode = true;
	if (FileSystem_MountContent(mountInfo) != FS_OK)
	{
		return Fail("cannot mount content");
	}

	// The resolved gameinfo.txt only has to exist while the search paths load.
	char tempDir[MAX_PATH];
	MakeTempDir(tempDir, sizeof(tempDir));
	const bool bWritten = WriteResolvedGameInfo(loadInfo.m_GameInfoPath, tempDir);
	FSReturnCode_t searchPathsResult = FS_INVALID_PARAMETERS;
	if (bWritten)
	{
		CFSSearchPathsInit searchInfo;
		searchInfo.m_pDirectoryName = tempDir;
		searchInfo.m_pFileSystem = loadInfo.m_pFileSystem;
		searchPathsResult = FileSystem_LoadSearchPaths(searchInfo);
	}
	char tempGameInfo[MAX_PATH];
	V_snprintf(tempGameInfo, sizeof(tempGameInfo), "%s/gameinfo.txt", tempDir);
	remove(tempGameInfo);
	RemoveTempDir(tempDir);
	if (!bWritten || searchPathsResult != FS_OK)
	{
		return bWritten ? Fail("cannot load the search paths of %s/gameinfo.txt", loadInfo.m_GameInfoPath) : false;
	}

	FileSystem_AddSearchPath_Platform(loadInfo.m_pFileSystem, loadInfo.m_GameInfoPath);
	V_strncpy(pszGameInfoDir, loadInfo.m_GameInfoPath, gameInfoDirSize);

	// Map-specific materials (cubemap and water patches) live in the map's own pakfile.
	loadInfo.m_pFileSystem->AddSearchPath(pszBspPath, "GAME", PATH_ADD_TO_HEAD);
	return true;
}

//-----------------------------------------------------------------------------
// vphysics, for surface properties and static prop collision, as the game sets it up
// (PhysParseSurfaceData) and as VBSP loads it.
//-----------------------------------------------------------------------------
bool InitPhysics(IPhysicsSurfaceProps *&pSurfaceProps, IPhysicsCollision *&pCollision)
{
	CSysModule *pPhysicsModule = Sys_LoadModule("vphysics");
	CreateInterfaceFn physicsFactory = pPhysicsModule ? Sys_GetFactory(pPhysicsModule) : nullptr;
	if (!physicsFactory)
	{
		return Fail("cannot load vphysics (is the tool next to it?)");
	}
	pSurfaceProps = static_cast<IPhysicsSurfaceProps *>(physicsFactory(VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, nullptr));
	pCollision = static_cast<IPhysicsCollision *>(physicsFactory(VPHYSICS_COLLISION_INTERFACE_VERSION, nullptr));
	if (!pSurfaceProps || !pCollision)
	{
		return Fail("vphysics lacks the surface property or collision interface");
	}

	KeyValues::AutoDelete pManifest(kSurfacePropManifest);
	if (!pManifest->LoadFromFile(g_pBakeFileSystem, kSurfacePropManifest, "GAME"))
	{
		return Fail("cannot read %s", kSurfacePropManifest);
	}
	for (KeyValues *pFile = pManifest->GetFirstSubKey(); pFile; pFile = pFile->GetNextKey())
	{
		if (V_stricmp(pFile->GetName(), "file") != 0)
		{
			continue;
		}
		CUtlBuffer text(0, 0, CUtlBuffer::TEXT_BUFFER);
		if (!g_pBakeFileSystem->ReadFile(pFile->GetString(), "GAME", text))
		{
			Warning("neo_soundbake: cannot read %s, skipping it\n", pFile->GetString());
			continue;
		}
		text.PutChar('\0');
		pSurfaceProps->ParseSurfaceData(pFile->GetString(), static_cast<const char *>(text.Base()));
	}
	return true;
}

//-----------------------------------------------------------------------------
// Writing the game lump. The game lump directory (LUMP_GAME_LUMP) holds absolute file offsets, so
// it is rewritten whole with every other game lump copied byte for byte. It goes in place when it
// already ends the file (a previous bake), else at the end of the file: nothing else moves.
//-----------------------------------------------------------------------------
bool ReadWholeFile(const char *pszPath, std::vector<uint8_t> &out)
{
	FILE *pFile = fopen(pszPath, "rb");
	if (!pFile)
	{
		return false;
	}
	fseek(pFile, 0, SEEK_END);
	const long size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	out.resize(size > 0 ? static_cast<size_t>(size) : 0);
	const bool bRead = size > 0 && fread(out.data(), 1, out.size(), pFile) == out.size();
	fclose(pFile);
	return bRead;
}

bool WriteProbeGameLump(const char *pszBspPath, const uint8_t *pLump, uint32_t lumpSize)
{
	std::vector<uint8_t> file;
	if (!ReadWholeFile(pszBspPath, file) || file.size() < sizeof(dheader_t))
	{
		return Fail("cannot read %s", pszBspPath);
	}
	dheader_t header;
	memcpy(&header, file.data(), sizeof(header));
	lump_t &gameLump = header.lumps[LUMP_GAME_LUMP];
	if (gameLump.uncompressedSize != 0 || gameLump.fileofs < 0 || gameLump.filelen < 0
		|| static_cast<size_t>(gameLump.fileofs) + gameLump.filelen > file.size())
	{
		return Fail("%s has a malformed game lump directory", pszBspPath);
	}

	// Every game lump but a previous bake's, with its data.
	struct KeptLump
	{
		dgamelump_t entry;
		std::vector<uint8_t> data;
	};
	std::vector<KeptLump> kept;
	if (gameLump.filelen >= static_cast<int>(sizeof(dgamelumpheader_t)))
	{
		dgamelumpheader_t directory;
		memcpy(&directory, file.data() + gameLump.fileofs, sizeof(directory));
		if (directory.lumpCount < 0
			|| directory.lumpCount > (gameLump.filelen - static_cast<int>(sizeof(directory))) / static_cast<int>(sizeof(dgamelump_t)))
		{
			return Fail("%s has a malformed game lump directory", pszBspPath);
		}
		for (int i = 0; i < directory.lumpCount; ++i)
		{
			dgamelump_t entry;
			memcpy(&entry, file.data() + gameLump.fileofs + sizeof(directory) + i * sizeof(dgamelump_t), sizeof(entry));
			if (entry.id == NeoSpatial::PROBE_GAME_LUMP_ID || (entry.id == 0 && entry.filelen == 0))
			{
				continue; // the previous bake, or the terminal entry compression appends
			}
			if (entry.flags & GAMELUMPFLAG_COMPRESSED)
			{
				return Fail("%s has compressed game lumps; run neo_soundbake before compressing the map (bspzip -repack -compress)",
							pszBspPath);
			}
			if (entry.fileofs < 0 || entry.filelen < 0 || static_cast<size_t>(entry.fileofs) + entry.filelen > file.size())
			{
				return Fail("%s has a malformed game lump", pszBspPath);
			}
			KeptLump keptLump;
			keptLump.entry = entry;
			keptLump.data.assign(file.begin() + entry.fileofs, file.begin() + entry.fileofs + entry.filelen);
			kept.push_back(std::move(keptLump));
		}
	}

	// Replace the directory in place when it ends the file; else leave it, unreferenced, and append.
	const bool bAtEnd = gameLump.filelen > 0 && static_cast<size_t>(gameLump.fileofs) + gameLump.filelen == file.size();
	file.resize(bAtEnd ? static_cast<size_t>(gameLump.fileofs) : file.size());
	file.resize((file.size() + 3) & ~static_cast<size_t>(3), 0); // lumps are 4-byte aligned

	dgamelump_t probeEntry = {};
	probeEntry.id = NeoSpatial::PROBE_GAME_LUMP_ID;
	probeEntry.version = NeoSpatial::PROBE_GAME_LUMP_VERSION;
	probeEntry.filelen = static_cast<int>(lumpSize);

	const size_t blockStart = file.size();
	const int count = static_cast<int>(kept.size()) + 1;
	size_t dataOffset = blockStart + sizeof(dgamelumpheader_t) + count * sizeof(dgamelump_t);
	std::vector<dgamelump_t> entries;
	for (KeptLump &keptLump : kept)
	{
		keptLump.entry.fileofs = static_cast<int>(dataOffset);
		dataOffset += keptLump.data.size();
		entries.push_back(keptLump.entry);
	}
	probeEntry.fileofs = static_cast<int>(dataOffset);
	entries.push_back(probeEntry);
	if (dataOffset + lumpSize > static_cast<size_t>(INT32_MAX))
	{
		return Fail("%s would grow past 2 GB", pszBspPath);
	}

	const dgamelumpheader_t directory = { count };
	const uint8_t *pDirectory = reinterpret_cast<const uint8_t *>(&directory);
	file.insert(file.end(), pDirectory, pDirectory + sizeof(directory));
	const uint8_t *pEntries = reinterpret_cast<const uint8_t *>(entries.data());
	file.insert(file.end(), pEntries, pEntries + entries.size() * sizeof(dgamelump_t));
	for (const KeptLump &keptLump : kept)
	{
		file.insert(file.end(), keptLump.data.begin(), keptLump.data.end());
	}
	file.insert(file.end(), pLump, pLump + lumpSize);

	gameLump.fileofs = static_cast<int>(blockStart);
	gameLump.filelen = static_cast<int>(file.size() - blockStart);
	memcpy(file.data(), &header, sizeof(header));

	// Through a temporary file, so a failed write never leaves a broken map behind.
	char tempPath[MAX_PATH];
	V_snprintf(tempPath, sizeof(tempPath), "%s.soundbake", pszBspPath);
	FILE *pOut = fopen(tempPath, "wb");
	bool bWritten = pOut && fwrite(file.data(), 1, file.size(), pOut) == file.size();
	if (pOut)
	{
		bWritten = (fclose(pOut) == 0) && bWritten;
	}
#ifdef _WIN32
	if (!bWritten || !MoveFileExA(tempPath, pszBspPath, MOVEFILE_REPLACE_EXISTING))
#else
	if (!bWritten || rename(tempPath, pszBspPath) != 0)
#endif
	{
		remove(tempPath);
		return Fail("cannot write %s", pszBspPath);
	}
	return true;
}

// Reloads the written map the way the game does (CNeoHrtfSystem::LoadBakedProbeLump), so a bake
// that would not load in game fails here rather than silently falling back at runtime.
bool VerifyBakedMap(const char *pszBspPath, const CNeoAudioGeometry::Services &services, NeoSpatial::ISpatializer &spatializer,
					const NeoSpatial::ProbeSet &baked)
{
	char error[512];
	CNeoAudioGeometry geometry;
	if (!geometry.LoadFromBsp(services, pszBspPath, error, sizeof(error)))
	{
		return Fail("the written map does not load: %s", error);
	}
	NeoSpatial::ProbeLumpHeader header;
	NeoSpatial::ProbeSet probes;
	CUtlVector<uint8> storage;
	const uint8 *pBatches = nullptr;
	if (!geometry.ReadBakedProbes(header, probes, storage, pBatches, error, sizeof(error)))
	{
		return Fail("the written map's baked probes do not read back: %s", error[0] ? error : "no probe lump");
	}
	if (probes.centres.size() != baked.centres.size() || probes.batchStarts != baked.batchStarts
		|| probes.batchAreas != baked.batchAreas)
	{
		return Fail("the written map's baked probes differ from the bake");
	}
	if (!spatializer.LoadProbeBatches(pBatches, header.batchesSize, error, sizeof(error))
		|| spatializer.GetBakeState(nullptr) != NeoSpatial::BakeState::Done)
	{
		return Fail("the written map's baked probes do not load: %s", error[0] ? error : "incomplete bake");
	}
	return true;
}

int Run(const char *pszBspArg)
{
	char bspPath[MAX_PATH];
	V_MakeAbsolutePath(bspPath, sizeof(bspPath), pszBspArg);
	V_FixSlashes(bspPath);
	const double startTime = Plat_FloatTime();

	char gameInfoDir[MAX_PATH];
	IPhysicsSurfaceProps *pSurfaceProps = nullptr;
	IPhysicsCollision *pCollision = nullptr;
	if (!InitFileSystem(bspPath, gameInfoDir, sizeof(gameInfoDir)) || !InitPhysics(pSurfaceProps, pCollision))
	{
		return 1;
	}

	char error[512];
	CNeoAudioGeometry geometry;
	const CNeoAudioGeometry::Services services = { g_pBakeFileSystem, pSurfaceProps, pCollision };
	if (!geometry.LoadFromBsp(services, bspPath, error, sizeof(error)))
	{
		Fail("%s", error);
		return 1;
	}
	Msg("neo_soundbake: %s: %d triangles\n", bspPath, geometry.NumTriangles());

	// The phonon the game ships, unless told otherwise.
	char phononPath[MAX_PATH];
	if (const char *pszPhonon = CommandLine()->ParmValue("-phonon"))
	{
		V_strncpy(phononPath, pszPhonon, sizeof(phononPath));
	}
	else
	{
		V_snprintf(phononPath, sizeof(phononPath), "%s/bin" PLATFORM_DIR "/%s", gameInfoDir, kPhononLibrary);
	}
	V_FixSlashes(phononPath);
	NeoSpatial::ISpatializer *pSpatializer =
		NeoSpatial::CreateSteamAudioSpatializer(phononPath, kBakeSampleRate, kBakeFrameSize, error, sizeof(error));
	if (!pSpatializer)
	{
		Fail("%s", error);
		return 1;
	}

	int result = 1;
	NeoSpatial::ProbeSet probes;
	if (!pSpatializer->SetSceneGeometry(geometry.GetSceneGeometry(), error, sizeof(error)))
	{
		Fail("%s", error);
	}
	else
	{
		NeoSpatial::BuildProbes(geometry.GetSceneGeometry(), geometry.GetBspTree(), NeoSpatial::DEFAULT_PROBE_SETTINGS, probes);
		if (probes.centres.empty())
		{
			Fail("the map has no open space to place probes in");
		}
		else if (!pSpatializer->SetProbeBatches(probes.Layout(), error, sizeof(error)))
		{
			Fail("%s", error);
		}
		else
		{
			const int threads = Max(1, CommandLine()->ParmValue("-threads", GetCPUInformation()->m_nLogicalProcessors));
			Msg("neo_soundbake: baking %d probes in %d batches on %d threads\n", static_cast<int>(probes.centres.size()),
				probes.NumBatches(), threads);
			pSpatializer->StartBake(threads);
			float progress = 0.0f;
			float reported = 0.0f;
			while (pSpatializer->GetBakeState(&progress) == NeoSpatial::BakeState::Running)
			{
				if (progress >= reported + kBakeProgressStep)
				{
					reported = progress;
					Msg("neo_soundbake: %3.0f%%\n", progress * 100.0f);
				}
				ThreadSleep(kBakePollMs);
			}

			int64_t batchesSize = 0;
			const uint8_t *pBatches = (pSpatializer->GetBakeState(nullptr) == NeoSpatial::BakeState::Done)
				? pSpatializer->SerializeProbeBatches(&batchesSize) : nullptr;
			if (!pBatches)
			{
				Fail("the bake did not finish");
			}
			else
			{
				std::vector<uint8_t> payload;
				NeoSpatial::WriteProbeLump(probes, pBatches, batchesSize, geometry.GetShapeCrc(), pSpatializer->GetLibraryVersion(),
										   payload);
				unsigned int compressedSize = 0;
				unsigned char *pCompressed = (payload.size() < UINT32_MAX)
					? LZMA_Compress(payload.data(), static_cast<unsigned int>(payload.size()), &compressedSize) : nullptr;
				if (!pCompressed)
				{
					Fail("cannot compress the baked probes");
				}
				else
				{
					if (WriteProbeGameLump(bspPath, pCompressed, compressedSize) && VerifyBakedMap(bspPath, services, *pSpatializer, probes))
					{
						Msg("neo_soundbake: wrote %.1f MB of baked probes (%.1f MB uncompressed) into %s in %.0f s\n",
							compressedSize / 1048576.0, payload.size() / 1048576.0, bspPath, Plat_FloatTime() - startTime);
						result = 0;
					}
					free(pCompressed);
				}
			}
		}
	}

	delete pSpatializer;
	return result;
}

} // namespace

// Plain console output, so Hammer's compile log shows it; errors go to stderr.
SpewRetval_t BakeSpew(SpewType_t spewType, const tchar *pszMessage)
{
	fputs(pszMessage, (spewType == SPEW_WARNING || spewType == SPEW_ERROR || spewType == SPEW_ASSERT) ? stderr : stdout);
	return (spewType == SPEW_ERROR) ? SPEW_ABORT : SPEW_CONTINUE;
}

int main(int argc, char **argv)
{
	SpewOutputFunc(BakeSpew);
	CommandLine()->CreateCmdLine(argc, argv);
	if (argc < 2 || CommandLine()->FindParm("-help") || argv[argc - 1][0] == '-')
	{
		Msg("usage: neo_soundbake -game <mod dir> [-threads <n>] [-phonon <library>] [-appid_dir_<appid> <dir>] <map.bsp>\n");
		return 1;
	}
	const int result = Run(argv[argc - 1]);
	if (g_pBakeFileSystem)
	{
		g_pBakeFileSystem->Shutdown();
	}
	return result;
}
