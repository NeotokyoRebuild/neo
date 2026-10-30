#pragma once

// Bounds how many debug overlays the engine is holding at once, by routing the
// engine's IVDebugOverlay interface through a budgeting proxy.
//
// An overlay is not a one-off draw: the engine keeps it until it expires and
// re-submits it to the material system as its own draw call on every frame it is
// alive. Nothing in the engine bounds how many may be alive at once, so a
// visualiser that emits per-entity and per-tick can hand the renderer more work
// than its dynamic-mesh pool can service, which fails as a null dereference
// inside materialsystem.dll far from the code that queued it.
//
// Wrapping the interface covers every overlay emitter in the game from a single
// place.

class IVDebugOverlay;

// Wrap the engine's debug overlay interface and return the pointer game code
// should use. A NULL input (a dedicated server has no debug overlay) returns
// NULL, so existing "if ( debugoverlay )" checks keep working.
IVDebugOverlay *NEO_InstallDebugOverlayBudget( IVDebugOverlay *pReal );
