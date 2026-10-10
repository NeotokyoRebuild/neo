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
     └──── SetVolumeByGuid(guid, 0) ────┤ every frame: origin → metres, gain, pitch, listener
                                        ▼ (one mutex-guarded voice table)
                         miniaudio device callback (audio thread), per 512-frame block per voice
                                        ▼
                  NeoSpatial::ISpatializer (neo_spatializer.h) ── neo_spatializer_steamaudio.cpp
```

| Layer | Files | Knows about |
| --- | --- | --- |
| Game integration | `neo_hrtf_system.{h,cpp}` | Source SDK, miniaudio decode/output |
| Contract | `neo_spatializer.h` | plain C++ only (metres, Source axes, float blocks) |
| Backend | `neo_spatializer_steamaudio.cpp` | `phonon.h`; loads `libphonon.so` / `phonon.dll` at runtime |
| Dependency | `src/cmake/steamaudio.cmake` | fetches the pinned SDK zip: headers + runtime library |

The backend has no Source SDK dependency, so it is built outside the unity build and PCH, and the
same file builds into an offline demo (`ntre/harness/hrtf/`). Steam Audio is never linked: if the
library is missing or fails to initialise, HRTF stays off with one warning (also shown by
`cl_neo_hrtf_debug 1`) and engine audio is untouched. Occlusion would extend the contract with
scene geometry.

## Trying it

1. Configure with `-DNEO_STEAMAUDIO=ON` (off by default). CMake then fetches the Steam Audio 4.8.1
   SDK once and copies its library next to `client` (`NEO_STEAMAUDIO_SDK_PATH` uses a local SDK).
2. In game, with headphones, on a server with `sv_cheats 1`: `cl_neo_hrtf 1`; A/B against the
   engine's own panning with `cl_neo_hrtf 0`. `cl_neo_hrtf_debug 1` overlays the status and a
   line per voice. Player pings and bots firing are easy sources.

`cl_neo_hrtf` is a cheat cvar, is not saved to config, and has no menu entry while the feature is
in development. With it at 0 (the default) the system does one cvar read per frame and never
touches engine sounds, the output device or Steam Audio, so normal sound is exactly master's.

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
- No occlusion or propagation.
- The first play of each file decodes on the game thread (a small hitch per new sound per map).
- One mutex guards the voice table for both threads, so a frame can wait on a block render and
  vice versa; POD double-buffering of voice parameters is the clean fix.
- MP3 player music is out of scope by design.
