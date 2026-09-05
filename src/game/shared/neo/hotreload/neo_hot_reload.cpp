// Linux hot reload glue (see neo_hot_reload.h). Wraps the vendored loader core
// with Source engine behavior: console logging, mode ConVars, the trigger and
// status commands, and the registry snapshot/rebuild around each apply
// (ServerClass/ClientClass chain, game systems, entity factories).
#include "cbase.h"

#ifdef NEO_LINUX_HOT_RELOAD

#include "neo_hot_reload.h"

#include "igamesystem.h"
#ifdef GAME_DLL
#include "server_class.h"
#include "NextBot/NextBotManager.h"
#else
#include "client_class.h"
#endif

#include "ntre_hr.h"

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
#define HR_PREFIX "sv_neo_hot_reload"
#define HR_MODULE "server"
#else
#define HR_PREFIX "cl_neo_hot_reload"
#define HR_MODULE "client"
#endif

#ifndef NEO_HOT_RELOAD_BUILD_DIR_REL
#error NEO_HOT_RELOAD_BUILD_DIR_REL must name the build dir relative to the module dir (set by CMake)
#endif
#ifndef NEO_HOT_RELOAD_BUILD_DIR_ABS
#error NEO_HOT_RELOAD_BUILD_DIR_ABS must name the absolute build dir (set by CMake)
#endif

static void HotReloadAutoChanged(IConVar *var, const char *pOldValue, float flOldValue);
static void HotReloadVerboseChanged(IConVar *var, const char *pOldValue, float flOldValue);

ConVar neo_hr_auto(HR_PREFIX "_auto", "1", FCVAR_DONTRECORD,
	"Apply hot reload shims as the sidecar publishes them. 0 = hold them until " HR_PREFIX ".",
	HotReloadAutoChanged);
ConVar neo_hr_verbose(HR_PREFIX "_verbose", "0", FCVAR_DONTRECORD,
	"Per-symbol hot reload detail in the console.",
	HotReloadVerboseChanged);

namespace {

ntre_hr *g_hr = nullptr;
bool g_inApply = false;

#ifdef GAME_DLL
CUtlVector<ServerClass *> g_classSnapshot;
#else
CUtlVector<ClientClass *> g_classSnapshot;
#endif
CUtlVector<IGameSystem *> g_systemSnapshot;
CUtlVector<ConCommandBase *> g_cvarSnapshot;

// Singletons a static object's constructor publishes into another translation unit's
// global. The shim's copy of that object runs its constructor at dlopen and would move
// the live pointer to an empty copy; the original is put back after every apply.
struct SingletonRestore
{
	const char *name;
	void *(*get)();
	void (*set)(void *);
	void *saved;
};
#ifdef GAME_DLL
void *GetNextBotSingleton() { return NextBotManager::GetInstance(); }
void SetNextBotSingleton(void *p) { NextBotManager::SetInstance(static_cast<NextBotManager *>(p)); }
#endif
SingletonRestore g_singletons[] = {
#ifdef GAME_DLL
	{ "NextBotManager::sInstance", GetNextBotSingleton, SetNextBotSingleton, nullptr },
#endif
	{ nullptr, nullptr, nullptr, nullptr },
};

void HotReloadSnapshotSingletons()
{
	for (SingletonRestore *r = g_singletons; r->name; ++r)
		r->saved = r->get();
}

int HotReloadRestoreSingletons()
{
	int restored = 0;
	for (SingletonRestore *r = g_singletons; r->name; ++r)
	{
		if (r->get() != r->saved)
		{
			r->set(r->saved);
			++restored;
		}
	}
	return restored;
}

void HotReloadLog(void *, ntre_hr_log_level level, const char *message)
{
	switch (level)
	{
	case NTRE_HR_LOG_DEBUG:
		DevMsg("[hotreload " HR_MODULE "] %s\n", message);
		break;
	case NTRE_HR_LOG_INFO:
		Msg("[hotreload " HR_MODULE "] %s\n", message);
		break;
	default:
		Warning("[hotreload " HR_MODULE "] %s\n", message);
		break;
	}
}

// Before the shim is mapped (its static ctors have not run): remember the
// registries the ctors are about to touch.
void HotReloadPreApply(void *, const ntre_hr_shim_info *)
{
	g_inApply = true;

	g_classSnapshot.RemoveAll();
#ifdef GAME_DLL
	for (ServerClass *sc = g_pServerClassHead; sc; sc = sc->m_pNext)
		g_classSnapshot.AddToTail(sc);
#else
	for (ClientClass *cc = g_pClientClassHead; cc; cc = cc->m_pNext)
		g_classSnapshot.AddToTail(cc);
#endif

	IGameSystem::HotReloadSnapshot(g_systemSnapshot);

	g_cvarSnapshot.RemoveAll();
	for (ConCommandBase *c = g_pCVar->GetCommands(); c; c = c->GetNext())
		g_cvarSnapshot.AddToTail(c);

	HotReloadSnapshotSingletons();
}

// A cvar we may unregister must belong to this game module or one of its live
// shims, never to the engine. dladdr maps an address back to the .so it came
// from: the base module (client.so / server.so) and every shim (client.<seq>.so)
// share the module-name prefix, while engine cvars such as snd_soundmixer resolve
// to engine.so and stay untouched. Pruning the whole shared registry is how an
// attach-time apply could reach engine-owned cvars at all; scoped to our own
// module it cannot.
static bool HotReloadCvarIsOurs(const ConCommandBase *c)
{
	Dl_info info;
	if (!dladdr(reinterpret_cast<const void *>(c), &info) || !info.dli_fname)
		return false;
	const char *base = strrchr(info.dli_fname, '/');
	base = base ? base + 1 : info.dli_fname;
	const size_t n = sizeof(HR_MODULE) - 1; // strlen of "client" / "server"
	return Q_strncmp(base, HR_MODULE, n) == 0 && base[n] == '.';
}

// The shim's static ctors re-register every ConVar and ConCommand its
// translation units define. A same-named duplicate is not benign: the engine
// dispatches the last registration, and for a ConCommand like say that drops
// the game-DLL client context (chat suddenly comes from "Console"). Keep the
// originals (their callbacks are hooked, so they already run the new code)
// and unregister the duplicates; genuinely new names stay, which is what
// makes "add a ConVar and reload" work. Collect first, then unregister, so the
// live list is walked once without being mutated underfoot, and only ever touch
// cvars this module owns.
int HotReloadPruneCvarDuplicates()
{
	CUtlVector<ConCommandBase *> dupes;
	for (ConCommandBase *c = g_pCVar->GetCommands(); c; c = c->GetNext())
	{
		if (g_cvarSnapshot.HasElement(c))
			continue;
		if (!HotReloadCvarIsOurs(c))
			continue;
		for (int i = 0; i < g_cvarSnapshot.Count(); ++i)
		{
			if (Q_stricmp(g_cvarSnapshot[i]->GetName(), c->GetName()) == 0)
			{
				dupes.AddToTail(c);
				break;
			}
		}
	}
	for (int i = 0; i < dupes.Count(); ++i)
		g_pCVar->UnregisterConCommand(dupes[i]);
	return dupes.Count();
}

// After hooks and relocations are in place: rebuild the class chain from the
// snapshot (the shim's duplicate ctor-inserted nodes drop out, the original
// nodes and their engine-assigned ids survive) and remove game systems the
// shim's ctors registered twice.
void HotReloadPostApply(void *, const ntre_hr_shim_info *shim, const ntre_hr_apply_counts *counts)
{
	if (g_classSnapshot.Count() > 0)
	{
		for (int i = 0; i < g_classSnapshot.Count() - 1; ++i)
			g_classSnapshot[i]->m_pNext = g_classSnapshot[i + 1];
		g_classSnapshot[g_classSnapshot.Count() - 1]->m_pNext = nullptr;
#ifdef GAME_DLL
		g_pServerClassHead = g_classSnapshot[0];
#else
		g_pClientClassHead = g_classSnapshot[0];
#endif
	}

	IGameSystem::HotReloadPrune(g_systemSnapshot);

	const int cvarsPruned = HotReloadPruneCvarDuplicates();
	const int singletonsRestored = HotReloadRestoreSingletons();

	g_inApply = false;

	Msg("[hotreload " HR_MODULE "] applied %s.%u: %u hooked, %u statics shared, %u copied, %u globals rebound, %u skipped, %d duplicate cvars pruned, %d singletons restored\n",
		shim->module, shim->seq, counts->functions_hooked, counts->statics_shared,
		counts->statics_copied, counts->globals_rebound, counts->functions_skipped, cvarsPruned, singletonsRestored);
}

// The module is loaded either straight out of the repo (Source SDK Base 2013 MP
// with -game <repo>/game/neo) or through the steamapps/sourcemods bind mount or
// symlink (the NT;RE entry in Steam). The build-dir path relative to the module
// covers the first case and any moved repo; walking up from sourcemods/neo
// leaves the repo, so that layout falls back to the configure-time absolute
// path, which is right whenever the game runs on the machine that built it.
const char *HotReloadBuildDir()
{
	Dl_info info;
	if (dladdr(reinterpret_cast<const void *>(&NeoHotReload_Init), &info) && info.dli_fname)
	{
		static char resolved[4096];
		snprintf(resolved, sizeof(resolved), "%s", info.dli_fname);
		if (char *slash = strrchr(resolved, '/'))
			*slash = '\0';
		const size_t dirLen = strlen(resolved);
		snprintf(resolved + dirLen, sizeof(resolved) - dirLen, "/%s", NEO_HOT_RELOAD_BUILD_DIR_REL);
		struct stat st;
		if (stat(resolved, &st) == 0 && S_ISDIR(st.st_mode))
			return NEO_HOT_RELOAD_BUILD_DIR_REL;
	}
	return NEO_HOT_RELOAD_BUILD_DIR_ABS;
}

bool HotReloadStatus(ntre_hr_status &st)
{
	memset(&st, 0, sizeof(st));
	st.struct_size = sizeof(st);
	return g_hr && ntre_hr_get_status(g_hr, &st);
}

void HotReloadTrigger()
{
	ntre_hr_status st;
	if (!HotReloadStatus(st))
	{
		Msg(HR_PREFIX ": hot reload is not active in this process\n");
		return;
	}
	if (!st.sidecar_attached)
	{
		Msg(HR_PREFIX ": no sidecar attached, run make watch in your build shell\n");
		return;
	}
	const uint32 applied = ntre_hr_apply_pending(g_hr);
	if (applied == 0)
		Msg(HR_PREFIX ": nothing pending (save a source file first)\n");
}

void HotReloadPrintStatus()
{
	ntre_hr_status st;
	if (!HotReloadStatus(st))
	{
		Msg(HR_PREFIX "_status: hot reload is not active in this process\n");
		return;
	}
	Msg("[hotreload " HR_MODULE "] %s\n", ntre_hr_version());
	Msg("  mailbox: %s\n", ntre_hr_mailbox_dir(g_hr));
	Msg("  build id: %s\n", ntre_hr_build_id(g_hr));
	if (st.sidecar_attached)
		Msg("  sidecar: attached (%s apply), seen %lld ms ago\n",
			st.sidecar_auto_apply ? "auto" : "trigger", (long long)st.sidecar_seen_ms_ago);
	else
		Msg("  sidecar: not attached, run make watch in your build shell\n");
	Msg("  mode: %s apply; shims applied %u (last seq %u), pending %u\n",
		st.auto_apply ? "auto" : "trigger", st.applied, st.applied_seq, st.pending);
	Msg("  region: %s, base 0x%llx, %u slots x %llu KiB, next slot %u%s\n",
		st.region_reserved ? (st.region_in_range ? "reserved in PC32 range" : "reserved OUT of range (copy mode statics)") : "NOT reserved",
		(unsigned long long)st.region_base, st.slot_count,
		(unsigned long long)(st.slot_size / 1024), st.next_slot,
		st.region_reserved ? "" : " (shims will not share statics)");
}

ConCommand neo_hr_trigger(HR_PREFIX, [](const CCommand &) { HotReloadTrigger(); },
	"Apply pending hot reload shims now.", FCVAR_DONTRECORD);
ConCommand neo_hr_status(HR_PREFIX "_status", [](const CCommand &) { HotReloadPrintStatus(); },
	"Hot reload state: sidecar, mode, applied shims, reserved region.", FCVAR_DONTRECORD);

} // namespace

static void HotReloadAutoChanged(IConVar *, const char *, float)
{
	if (g_hr)
		ntre_hr_set_auto_apply(g_hr, neo_hr_auto.GetBool());
}

static void HotReloadVerboseChanged(IConVar *, const char *, float)
{
	if (g_hr)
		ntre_hr_set_verbose(g_hr, neo_hr_verbose.GetBool());
}

void NeoHotReload_Init()
{
	if (g_hr)
		return;

	ntre_hr_config cfg;
	ntre_hr_config_init(&cfg);
	cfg.module_name = HR_MODULE;
	cfg.module_anchor = reinterpret_cast<const void *>(&NeoHotReload_Init);
	cfg.build_dir = HotReloadBuildDir();
	cfg.auto_apply = neo_hr_auto.GetBool();
	cfg.verbose = neo_hr_verbose.GetBool();
	cfg.log = HotReloadLog;
	cfg.pre_apply = HotReloadPreApply;
	cfg.post_apply = HotReloadPostApply;

	g_hr = ntre_hr_init(&cfg);
	if (g_hr)
		Msg("[hotreload " HR_MODULE "] ready (%s); mailbox %s\n",
			ntre_hr_version(), ntre_hr_mailbox_dir(g_hr));
	else
		Warning("[hotreload " HR_MODULE "] init failed, continuing without hot reload (see lines above)\n");
}

void NeoHotReload_Frame()
{
	if (!g_hr)
		return;
	ntre_hr_poll(g_hr);
}

void NeoHotReload_Shutdown()
{
	if (!g_hr)
		return;
	ntre_hr_shutdown(g_hr);
	g_hr = nullptr;
}

void NeoHotReload_LevelInitNotice()
{
	ntre_hr_status st;
	if (HotReloadStatus(st) && !st.sidecar_attached)
		Msg("[hotreload " HR_MODULE "] hot reload build, no sidecar attached; run make watch in your build shell\n");
}

bool NeoHotReload_InApply()
{
	return g_inApply;
}

#endif // NEO_LINUX_HOT_RELOAD
