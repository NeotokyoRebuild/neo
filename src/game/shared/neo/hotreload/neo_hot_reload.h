// Linux hot reload: Source-side glue around the vendored loader core
// (vendor/ntre_hr.h). One instance per module; the same TU is compiled into
// server.so (sv_neo_hot_reload_*) and client.so (cl_neo_hot_reload_*).
// Everything compiles away unless NEO_LINUX_HOT_RELOAD is defined
// (the linux-debug-hotreload preset).
#ifndef NEO_HOT_RELOAD_H
#define NEO_HOT_RELOAD_H
#ifdef _WIN32
#pragma once
#endif

#ifdef NEO_LINUX_HOT_RELOAD

// Create the loader context. Call once, late in module init (after ConVars are
// registered so the mode ConVars are live). Safe to call when it fails: the
// module just runs without hot reload and says why in the console.
void NeoHotReload_Init();

// Poll the mailbox. Call once per frame from the module's frame hook.
void NeoHotReload_Frame();

// Restore hooked entries and remove the state file. Call from module shutdown.
void NeoHotReload_Shutdown();

// One console line at level init when this is a hotreload build with no
// sidecar attached (tells the developer to run make watch).
void NeoHotReload_LevelInitNotice();

// True while a shim is being applied (between the pre and post apply
// callbacks). CEntityFactoryDictionary::InstallFactory uses this to keep the
// original factory when a shim's static ctors re-run LINK_ENTITY_TO_CLASS.
bool NeoHotReload_InApply();

#endif // NEO_LINUX_HOT_RELOAD

#endif // NEO_HOT_RELOAD_H
