# NEO HRTF — client-side spatial audio (proof of concept)

Binaural (HRTF) rendering of positional in-game sounds — other players' weapons, footsteps,
world sounds — without touching the closed-source Source engine mixer.

## Why it is built this way

The engine owns the mixer and plays most sounds itself, including every sound the server starts,
so `client.dll` never sees an audio buffer. Through public interfaces the client can:

- see every active engine channel each frame (`IEngineSound::GetActiveSounds()`: guid, file,
  source entity, origin, volume, pitch, flags), including server-started sounds;
- silence any one of them (`IEngineSound::SetVolumeByGuid()`);
- read the same `sound/` files through `IFileSystem`, and output audio through the vendored
  `miniaudio`.

So positional sounds are re-rendered in parallel: poll the channel list, mute the engine's copy,
decode the same file and play it through a second output device with HRTF applied.
Non-positional sounds (UI, music, sentences, the local player's own weapon) stay with the engine.

```
engine mixer ──GetActiveSounds()──► CNeoHrtfSystem (game thread, once per frame)
     ▲                                  │ new guid: resolve file, decode + cache, mute engine copy
     └──── SetVolumeByGuid(guid, 0) ────┤ every frame: origin → metres, gain, pitch, listener,
                                        │ then SimulateDirect → occlusion + transmission per voice
                                        ▼ (one mutex-guarded voice table)
                         miniaudio device callback (audio thread), per 512-frame block per voice
                                        ▼
                  NeoSpatial::ISpatializer (neo_spatializer.h) ── neo_spatializer_steamaudio.cpp
```

| Layer | Files | Knows about |
| --- | --- | --- |
| Game integration | `neo_hrtf_system.{h,cpp}` | Source SDK, miniaudio decode/output |
| Scene geometry | `neo_audio_geometry.{h,cpp}` | BSP lumps, VMT `$surfaceprop`, physics surface props |
| Probe placement | `neo_audio_probes.{h,cpp}` | plain C++: BSP tree in metres, the contract |
| Contract | `neo_spatializer.h` | plain C++ only (metres, Source axes, float blocks) |
| Backend | `neo_spatializer_steamaudio.cpp` | `phonon.h`; loads `libphonon.so` / `phonon.dll` at runtime |
| Dependency | `src/cmake/steamaudio.cmake` | fetches the pinned SDK zip: headers + runtime library |

The backend has no Source SDK dependency, so it is built outside the unity build and PCH, and the
same file builds into an offline demo (`ntre/harness/hrtf/`). Steam Audio is never linked: if the
library is missing or fails to initialise, HRTF stays off with one warning (also shown by
`cl_neo_hrtf_debug 1`) and engine audio is untouched.

## Acoustic scene

Occlusion, reflections and pathing all trace rays against the map, so each level gets a Steam
Audio scene (`IPLScene` holding one `IPLStaticMesh`). `CNeoAudioGeometry` reads it straight from
`maps/<map>.bsp` through `IFileSystem`:

- only the world model (model 0): brush entities can move, so they would need instanced meshes;
- brush faces (`LUMP_FACES`, or `LUMP_FACES_HDR` for HDR-only compiles) walked through surfedges,
  edges and vertices, fan-triangulated and re-wound counter-clockwise for Steam Audio;
- displacements tessellated from `LUMP_DISPINFO` / `LUMP_DISP_VERTS` the way the engine does;
- faces flagged sky, nodraw, trigger, hint or skip are left out (sound escapes through the sky);
- each texture's `$surfaceprop` (following patch materials) resolves to a physics surface and its
  `CHAR_TEX_*` game material, which picks one of Steam Audio's reference acoustic materials;
- solid static props from the `sprp` game lump (`LUMP_GAME_LUMP`): `SOLID_VPHYSICS` props use
  solid 0 of the model's `.phy` (loaded through `IPhysicsCollision`, one load per model, each
  convex re-wound outward), `SOLID_BBOX` props the `.mdl`'s hull box, both placed by the prop's
  origin, angles and (lump version 11+) uniform scale; the material comes from the solid's (or
  the `.mdl`'s) surfaceprop, and non-solid props are left out;
- LZMA-compressed lumps and game lumps are decompressed; every index is bounds-checked, so a malformed map only
  means no scene.

The scene is built at `LevelInitPreEntity` when HRTF is on, otherwise on the first in-game frame
after enabling it, so players without HRTF never read the BSP. It is released at level shutdown.
`cl_neo_hrtf_scene_obj` writes it to `<game dir>/hrtf_scene_<map>.obj` for inspection in any
model viewer.

## Occlusion

A direct-only `IPLSimulator` is created with each scene, with an `IPLSource` per live voice.
Once per frame, after the voice table is updated, the game thread runs `SimulateDirect`:

- volumetric occlusion: each source is a 0.3 m sphere sampled with 16 rays, so a sound fades in
  as its source rounds a corner instead of switching on;
- frequency-dependent transmission through up to 3 surfaces, from the materials above, so a sound
  through a wall is quieter and duller rather than silent;
- traced from 8 units above the sound's origin, since entity sounds (footsteps) play from the
  floor and would otherwise be half hidden by it;
- distance attenuation, air absorption and directivity stay with our engine-matching model.

The result reaches the audio thread as a POD `DirectPath` in the voice table, and `Process` runs
an `IPLDirectEffect` on the block before the binaural effect. A new voice is not rendered until
its first path is published (the same frame), so a shot behind a wall never starts unoccluded.
Simulating never shares state with `Process`, so the audio thread keeps rendering while it runs.

Measured cost: about 0.2 ms per frame for 32 voices against a 97k-triangle scene, which is why it
runs on the game thread. Scene build is roughly 80 ms for the same size, during map load.
`cl_neo_hrtf_occlusion 0` turns it off for A/B listening; `cl_neo_hrtf_debug 1` shows each
voice's occlusion and per-band transmission, plus the simulation time.

## Probes and baking

Baked reflections are looked up from probes: points where the acoustics are precomputed. Each
map gets them automatically when its scene is built (`neo_audio_probes.{h,cpp}`, plain C++):

- Steam Audio's own generator (`IPL_PROBEGENERATIONTYPE_UNIFORMFLOOR`) runs once per open BSP
  leaf (lump 10: not solid, inside the map, not in the 3D skybox's area), on the leaf's box pulled
  in 0.25 m from its walls. It places probes 2 m apart, 1.5 m above floors it finds by casting rays
  down, so displacement terrain counts as floor.
- That generator looks for floors up to `height` below the box, so a probe can land in a
  neighbouring leaf. Each probe is batched by the BSP area of the leaf that actually contains it
  (a BSP walk). A leaf hovering higher than that above the floor gets no probes.
- Each box gets its own centred grid, so probes from neighbouring leaves can nearly coincide.
  Any probe within 3/4 of the spacing of one already kept is dropped. Over real maps the mean
  nearest-neighbour distance comes out at 1.87–1.91 m for the 2 m spacing.
- One `IPLProbeBatch` per BSP area that ends up with probes: a region sealed off by
  areaportals, so usually one or two per map. Steam Audio indexes the probes inside a batch, so
  large batches cost nothing at lookup. Leaves and vis clusters were tried first, but on NT;RE
  maps every open leaf is its own cluster, which meant 200–450 tiny batches. Generation takes a
  few milliseconds.

Batches are baked with listener-centric parametric reverb (`IPL_BAKEDDATAVARIATION_REVERB`,
three decay times per probe) on a background thread. Convolution IRs would cost hundreds of KB
per probe. Pathing data (below) is baked into the same batches straight after. Full bakes
measured on 4 threads: oilstain (860 probes) 35 s, dawn (2,478) 44 s, ghost (3,938) 166 s, of which
pathing is 3–20 s. Pathing data grows with the number of probe pairs, so the cache is 2.9 MB,
23 MB and 35 MB respectively. By default (`cl_neo_hrtf_bake_auto 1`) a map
without a cached bake starts baking on load with a quarter of the logical cores
(`cl_neo_hrtf_bake_threads`). The cache is written only when the whole bake finishes, so leaving
the map mid-bake starts it over next time.

The result is saved to `hrtf/<map>.probes` under the mod directory. Its key covers the
geometry, materials, BSP leaves and probe layout, so a changed map rebakes; bump
`kHrtfProbeCacheVersion` whenever the bake parameters change. `cl_neo_hrtf_bake` rebakes,
`cl_neo_hrtf_bake_cancel` stops a bake, `cl_neo_hrtf_debug_probes 1` draws nearby probes
coloured by batch, and `cl_neo_hrtf_debug 1` shows the probe count and bake progress.

The same batches carry pathing: Steam Audio only finds paths between probes in the same batch,
and an area batch spans everything connected short of an areaportal.

## Reverb

The baked probes drive one room reverb around the listener:

- **Lookup (game thread, ~2 µs a frame):** the simulator also has `IPL_SIMULATIONFLAGS_REFLECTIONS`
  and a single baked source. Steam Audio looks reverb up at the listener's position by blending
  nearby probes, tracing no rays, and returns decay times (RT60) for three bands. Batches join the
  simulator only once fully baked (finished or loaded from the cache). A rebake detaches them first,
  so the simulator never reads a batch the bake thread is writing.
- **Away from probes:** mid-jump, say, the lookup finds nothing. The last room is held, because the
  effect would otherwise fall back to its 0.1 s minimum, a small fake room. Decay times are capped
  at 3 s: the bake simulates only 1 s of decay, so longer fits are extrapolations. On dawn one
  probe reported 10 s, while the median across the map is 0.6–0.7 s.
- **Send:** each voice's block after occlusion and transmission, times the voice's gain, is summed
  into one bus. A distant or walled-off sound excites the room only as much as it is heard. Steam
  Audio's own listener reverb is fed the same way.
- **Render (audio thread):** one parametric `IPLReflectionEffect` (a feedback delay network) per
  block, even with no voices, so tails ring out. It has a single output; Steam Audio itself decodes
  that as omnidirectional, the same in both ears. Instead each ear gets the tail through its own
  chain of three Schroeder all-pass filters: flat in magnitude, different in phase. In testing the
  ears came out decorrelated (correlation 0.02) at equal energy, so the room surrounds the listener
  instead of sitting inside the head.

`cl_neo_hrtf_reverb` sets the level (default 1, 0 turns it off). `cl_neo_hrtf_reverb_inhead 1`
skips the all-passes and sends the same tail to both ears, inside the head, at the same level. Until a map's bake finishes
there is no reverb, and the debug overlay says so. `cl_neo_hrtf_debug 1` shows the current RT60s.

## Pathing

Occlusion alone makes a sound behind a wall quieter and duller. Pathing makes it come around the
wall instead, from the doorway it actually reaches the listener through.

- **Bake:** after the reverb, each batch gets `IPL_BAKEDDATATYPE_PATHING`. That records probe
  pairs that see each other (more than 10% of 4×4 rays between 0.5 m spheres get through), at most
  20 m apart, with shortest paths through them up to 100 m long.
- **Simulation (game thread, ~0.1–0.2 ms for 32 voices):** each voice's source also runs pathing
  against the batch of the listener's BSP area. A voice in another area, behind an areaportal, has
  no path. Paths aren't re-validated per frame, since the world they were baked against is static.
  Each voice's distance law is passed in as a callback, so a path is attenuated by the engine-matching
  law at its own length: around a corner is quieter than in view at the same straight-line distance.
- **Render (audio thread):** only the share the direct path does not carry, `1 - occlusion`, goes
  along the paths, ramped across each block. A visible source is never doubled, and stepping
  behind cover crossfades from direct to pathed. Each voice's `IPLPathEffect` rotates the paths'
  first-order sound field to the listener and renders it binaurally, so the sound arrives from the
  opening. Its deviation EQ is not normalised, so bending around an obstacle costs level
  (Steam Audio's default model: roughly -5/-13/-18 dB low/mid/high around a wall's end).
  Normalising it made occluded sounds on open maps such as oilstain nearly as loud as in view.
- **Only while covered.** Steam Audio recomputes a voice's paths only while both the voice and the
  listener are inside some probe's 2 m sphere of influence. Otherwise it silently returns the last
  ones, and those stayed audible at a constant level however far the listener walked. On oilstain,
  two of the three `skylines.wav` emitters hang 2.1 and 4.5 m from the nearest probe (probes are
  1.5 m above floors; the emitters 4–7 m up), so their paths froze at the last line-of-sight value.
  Now any end outside every sphere is pulled just inside the nearest probe's sphere within 8 m
  (`ProbeCoverage::PullInside`, a hash grid over the probes). Pathing is skipped when no probe is
  that close. Paths start from the raised occlusion origin.
- **Capped.** A path's level is capped at the voice's straight-line level (a path is never shorter),
  so no simulation result can be louder than the sound in plain view.
- **Steam Audio's own Unity spatializer adds pathing on top of the direct sound.** Its paths include
  the direct line whenever the source is visible, which would double a visible source; hence the
  `1 - occlusion` split.

Known gaps: from a fixed source on oilstain, 4–6% of occluded listener positions within 40 m get
no path at all, while their neighbours do, so a pathed sound can drop out briefly while walking.

`cl_neo_hrtf_pathing 0` turns it off for A/B listening. `cl_neo_hrtf_debug 1` shows each voice's
path level.

Not done: real-time reflections (discrete echoes, and reverb that follows moving sources).

## Trying it

1. Build normally. CMake fetches the Steam Audio 4.8.1 SDK once and copies its library next to
   `client` (`NEO_STEAMAUDIO=OFF` builds without HRTF; `NEO_STEAMAUDIO_SDK_PATH` uses a local SDK).
2. In game, with headphones: `cl_neo_hrtf 1`; A/B against the engine's own panning with
   `cl_neo_hrtf 0`. `cl_neo_hrtf_debug 1` overlays the status and a line per voice. Player pings
   and bots firing are easy sources; walk behind a wall and toggle `cl_neo_hrtf_occlusion`.

## Updating Steam Audio

The `steam-audio` repo is kept as an untouched sibling mirror of `ValveSoftware/steam-audio`
(fast-forward it to take upstream). neo consumes only the release artefacts: bump the URL and
SHA256 in `src/cmake/steamaudio.cmake` and rebuild. A renamed phonon function shows up as a named
missing symbol in the load error, not as a compile break in game code. To debug inside Steam
Audio, build it from that checkout (`core/doc/build-instructions.rst`: `get_dependencies.py`,
then `cmake --build ... --target install`) and configure neo with
`-DNEO_STEAMAUDIO_SDK_PATH=<tree with include/ and lib/<plat>/>`.

## Known limitations of the proof of concept

- The engine's copy plays for up to one frame before it is muted, briefly doubling the attack.
  Muting at emission would need engine code.
- Distance attenuation re-implements the engine's model from the sound script's `soundlevel`
  (raw filenames use `SNDLVL_NORM`); engine DSP, ducking and room reverb do not apply.
- Reverb is one listener-centric room, not per-source reflections. A sound in another room
  reverberates in the listener's room only as much as it gets through the wall (pathed sound does
  not feed the reverb). Paths
  stop at areaportals and at 100 m. The scene has no brush entities (doors, func_brush) or
  dynamic props, so cover made of those does not occlude.
- The first play of each file decodes on the game thread (a small hitch per new sound per map).
- One mutex guards the voice table for both threads, so a frame can wait on a block render and
  vice versa; POD double-buffering of voice parameters is the clean fix.
- MP3 player music is out of scope by design.
