#include "cbase.h"
#include "neo_hrtf_system.h"
#include "neo_audio_geometry.h"
#include "neo_audio_probe_lump.h"
#include "c_neo_ambient_generic.h"

#include "checksum_crc.h"
#include "engine/IEngineSound.h"
#include "engine/ivdebugoverlay.h"
#include "filesystem.h"
#include "soundchars.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "utlbuffer.h"
#include "view.h"

#include "miniaudio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;
extern IPhysicsSurfaceProps *physprops;
extern IPhysicsCollision *physcollision;

namespace
{

constexpr int kHrtfOutputChannels = 2;
constexpr float kHrtfMetresPerUnit = 0.0254f; // 1 Source unit = 1 inch
constexpr int kHrtfMaxCachedSeconds = 30; // longer sounds (music-like loops) stay with the engine
constexpr int kHrtfMaxCachedFrames = kHrtfMaxCachedSeconds * CNeoHrtfSystem::kSampleRate;

// The engine mixer (snd_dma.cpp) attenuates with an inverse-distance law normalised so a
// sound at its scripted soundlevel is at unity gain snd_refdist units away:
//   dist_mult = 10^((snd_refdb - sndlvl) / 20) / snd_refdist,  gain = 1 / (dist * dist_mult)
// clamped to [snd_gain_min, snd_gain_max]. These are the engine's defaults for those cvars.
constexpr float kHrtfEngineRefDb = 60.0f;
constexpr float kHrtfEngineRefDistUnits = 36.0f;
constexpr float kHrtfEngineGainMax = 1.0f;
constexpr float kHrtfEngineGainMin = 0.01f;

// Entity sounds play from the entity origin, which for players is on the floor; occlusion is
// traced from slightly above it so the floor itself does not hide half the source.
constexpr float kHrtfOcclusionLiftUnits = 8.0f;

// A path's omnidirectional coefficient for a source at unit gain: the order-0 spherical harmonic,
// 1 / (2 sqrt(pi)), which is what Steam Audio returns for a source in plain view.
constexpr float kHrtfFirstOrderOmniGain = 0.28209479f;

// How far a sound (or the listener) may be from the nearest probe and still be pathed, from a point
// pulled into that probe's sphere. Covers ambient emitters hung a few metres up and a jumping or
// ledge-standing listener; beyond it the nearest probe likely describes another space.
constexpr float kHrtfPathReachMetres = 8.0f;


// Baked probes are cached per map under the mod directory, keyed by geometry and probe layout.
// Bump the version whenever the bake parameters in the backend or the file layout change.
constexpr char kHrtfProbeCacheDir[] = "hrtf";
constexpr uint32 kHrtfProbeCacheMagic = 0x4350484e; // "NHPC"
constexpr uint32 kHrtfProbeCacheVersion = 2; // 2: pathing baked alongside reverb

struct HrtfProbeCacheHeader
{
	uint32 magic;
	uint32 version;
	uint32 key;
	uint32 reserved;
};

// A reported engine volume this close to the one we set is taken to be ours read back, allowing
// for the engine storing it as 0-255.
constexpr float kHrtfVolumeReadbackTolerance = 1.0f / 255.0f;

// The engine keeps channel volume in steps of 1/255, rounding down, so a volume we set is rounded
// up to a whole step to stay at least as loud as asked.
constexpr float kHrtfEngineVolumeStep = 1.0f / 255.0f;

// The engine copy's volume is raised as soon as its gain calls for it, but lowered only once the
// target falls well below it, so it is not reset every poll while the gain drifts.
constexpr float kHrtfMuteLowerRatio = 0.7f;

// Leave most cores to the game while a bake runs in the background.
constexpr int kHrtfBakeCoresPerThread = 4;

constexpr float kHrtfProbeDrawRangeUnits = 1024.0f;
constexpr float kHrtfProbeDrawSizeUnits = 4.0f;

#ifdef _WIN32
constexpr char kHrtfPhononLibrary[] = "bin/x64/phonon.dll";
#else
constexpr char kHrtfPhononLibrary[] = "bin/linux64/libphonon.so";
#endif

constexpr char kHrtfSoundDir[] = "sound/";

// RIFF layout (little endian): "RIFF" size "WAVE", then chunks of id(4) size(4) body, each
// body padded to an even length.
constexpr int kRiffHeaderSize = 12;
constexpr int kRiffChunkHeaderSize = 8;
constexpr int kWavFmtSampleRateOffset = 4;
constexpr int kWavCueCountSize = 4;
constexpr int kWavCuePointSize = 24;
constexpr int kWavCueSampleOffsetOffset = 20;
constexpr int kWavSmplHeaderSize = 36;
constexpr int kWavSmplLoopCountOffset = 28;
constexpr int kWavSmplLoopSize = 24;
constexpr int kWavSmplLoopStartOffset = 8;
constexpr int kWavSmplLoopEndOffset = 12;

uint32 HrtfReadLE32(const uint8 *pData)
{
	return uint32(pData[0]) | (uint32(pData[1]) << 8) | (uint32(pData[2]) << 16) | (uint32(pData[3]) << 24);
}

// Source loops a wav from the sample offset of its first cue point (CAudioSourceWave::
// ParseCueChunk) or the start of its first sampler loop (ParseSamplerChunk), whichever chunk
// comes last, so do the same. A sampler loop also ends the sound after its last sample.
// Outputs are in output-rate frames: loopStart is -1 for one-shots, loopEnd (exclusive) is -1
// when the sound plays to the end of its data.
void HrtfParseWavLoop(const uint8 *pData, int size, int &loopStart, int &loopEnd)
{
	loopStart = -1;
	loopEnd = -1;
	if (size < kRiffHeaderSize || V_memcmp(pData, "RIFF", 4) != 0 || V_memcmp(pData + 8, "WAVE", 4) != 0)
	{
		return;
	}

	uint32 sourceRate = 0;
	int64 loopSampleOffset = -1;
	int64 loopEndSampleOffset = -1;
	for (int64 pos = kRiffHeaderSize; pos + kRiffChunkHeaderSize <= size;)
	{
		const uint8 *pChunk = pData + pos;
		const uint32 chunkSize = HrtfReadLE32(pChunk + 4);
		const uint8 *pBody = pChunk + kRiffChunkHeaderSize;
		if (pos + kRiffChunkHeaderSize + chunkSize > size)
		{
			break;
		}

		if (V_memcmp(pChunk, "fmt ", 4) == 0 && chunkSize >= kWavFmtSampleRateOffset + sizeof(uint32))
		{
			sourceRate = HrtfReadLE32(pBody + kWavFmtSampleRateOffset);
		}
		else if (V_memcmp(pChunk, "cue ", 4) == 0 && chunkSize >= kWavCueCountSize + kWavCuePointSize
				 && HrtfReadLE32(pBody) > 0)
		{
			loopSampleOffset = HrtfReadLE32(pBody + kWavCueCountSize + kWavCueSampleOffsetOffset);
		}
		else if (V_memcmp(pChunk, "smpl", 4) == 0 && chunkSize >= kWavSmplHeaderSize + kWavSmplLoopSize
				 && HrtfReadLE32(pBody + kWavSmplLoopCountOffset) > 0)
		{
			loopSampleOffset = HrtfReadLE32(pBody + kWavSmplHeaderSize + kWavSmplLoopStartOffset);
			// The stored end is the loop's last sample, so one past it is the exclusive end.
			loopEndSampleOffset = int64(HrtfReadLE32(pBody + kWavSmplHeaderSize + kWavSmplLoopEndOffset)) + 1;
		}
		pos += kRiffChunkHeaderSize + chunkSize + (chunkSize & 1);
	}

	if (sourceRate == 0 || loopSampleOffset < 0)
	{
		return;
	}
	loopStart = static_cast<int>(loopSampleOffset * CNeoHrtfSystem::kSampleRate / sourceRate);
	if (loopEndSampleOffset > loopSampleOffset)
	{
		loopEnd = static_cast<int>(loopEndSampleOffset * CNeoHrtfSystem::kSampleRate / sourceRate);
	}
}

// Soundscripts and engine channels both reduce to this form: sound chars stripped,
// lower case, forward slashes, relative to sound/.
// The engine's dist_mult for a sound level (see kHrtfEngineRefDb); SNDLVL_NONE means "heard
// everywhere": no distance falloff at all.
float HrtfDistMultForLevel(soundlevel_t level)
{
	if (level == SNDLVL_NONE)
	{
		return 0.0f;
	}
	return powf(10.0f, (kHrtfEngineRefDb - static_cast<float>(level)) / 20.0f) / kHrtfEngineRefDistUnits;
}

void HrtfNormaliseSoundName(const char *pszName, char *pszOut, int outSize)
{
	V_strncpy(pszOut, PSkipSoundChars(pszName), outSize);
	V_FixSlashes(pszOut, '/');
	V_strlower(pszOut);
	constexpr int soundDirLen = sizeof(kHrtfSoundDir) - 1;
	if (V_strncmp(pszOut, kHrtfSoundDir, soundDirLen) == 0)
	{
		V_memmove(pszOut, pszOut + soundDirLen, V_strlen(pszOut + soundDirLen) + 1);
	}
}

// Streams are not cached; omni sounds are non-directional by design; doppler, directional
// and distance-variant sounds are encoded stereo pairs rather than mono sources.
bool HrtfIsEngineOnlyName(const char *pszRawName)
{
	return TestSoundChar(pszRawName, CHAR_STREAM) || TestSoundChar(pszRawName, CHAR_OMNI)
		|| TestSoundChar(pszRawName, CHAR_DOPPLER) || TestSoundChar(pszRawName, CHAR_DIRECTIONAL)
		|| TestSoundChar(pszRawName, CHAR_DISTVARIANT);
}

bool HrtfIsSpatialCandidate(const SndInfo_t &info, int localPlayerIndex)
{
	// Sentences, dry-mix and speaker sounds are not positional; the local player's own weapon and
	// viewmodel sounds are non-positional by design; UI sounds have no source entity or position.
	return !info.m_bIsSentence && !info.m_bDryMix && !info.m_bSpeaker && info.m_pOrigin
		&& info.m_nSoundSource != localPlayerIndex && (info.m_nSoundSource > 0 || *info.m_pOrigin != vec3_origin);
}

NeoSpatial::Vec3 HrtfToVec3(const Vector &v, float scale = 1.0f)
{
	return { v.x * scale, v.y * scale, v.z * scale };
}

void HrtfDataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	(void)pInput;
	static_cast<CNeoHrtfSystem *>(pDevice->pUserData)->Render(static_cast<float *>(pOutput), static_cast<int>(frameCount));
}

} // namespace

ConVar cl_neo_hrtf("cl_neo_hrtf", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Re-render positional sounds with HRTF (proof of concept)", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_volume("cl_neo_hrtf_volume", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Volume of the HRTF output, on top of the master volume", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_debug("cl_neo_hrtf_debug", "0", FCVAR_CLIENTDLL,
	"Overlay the HRTF status and one line per voice: file, distance, gain, azimuth, occlusion", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_occlusion("cl_neo_hrtf_occlusion", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Muffle HRTF sounds by the map geometry between them and the listener", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_pathing("cl_neo_hrtf_pathing", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Let occluded HRTF sounds reach the listener around obstacles, along paths baked into the map's probes", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_reverb("cl_neo_hrtf_reverb", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Level of the room reverb baked into the map's acoustic probes, 0 = off", true, 0.0f, true, 2.0f);
ConVar cl_neo_hrtf_reverb_inhead("cl_neo_hrtf_reverb_inhead", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Play the room reverb identically in both ears (inside the head) instead of decorrelated around the listener", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_bake_auto("cl_neo_hrtf_bake_auto", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Bake the map's acoustic probes in the background when there is no cached bake for it", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_bake_threads("cl_neo_hrtf_bake_threads", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Threads for baking acoustic probes, 0 = a quarter of the logical cores", true, 0.0f, true, 64.0f);
ConVar cl_neo_hrtf_debug_probes("cl_neo_hrtf_debug_probes", "0", FCVAR_CLIENTDLL,
	"Draw the acoustic probes near the view, coloured by batch (BSP area)", true, 0.0f, true, 1.0f);
// The engine frees a one-shot channel once its spatialized volume falls below about 3/255 (measured
// between 0.0082 and 0.0136), so the engine's copy of an HRTF sound is held just above that instead
// of silenced, with some headroom for its gain dropping between polls.
ConVar cl_neo_hrtf_engine_floor("cl_neo_hrtf_engine_floor", "0.016", FCVAR_CLIENTDLL,
	"Spatialized level the engine's copy of an HRTF sound is held at so the engine does not cut it off early, 0 = silent", true, 0.0f, true, 0.1f);

static CNeoHrtfSystem s_neoHrtfSystem;

CON_COMMAND(cl_neo_hrtf_scene_obj, "Write the HRTF acoustic scene (the map's world geometry) to <game dir>/hrtf_scene_<map>.obj")
{
	s_neoHrtfSystem.SaveSceneObj();
}

CON_COMMAND(cl_neo_hrtf_bake, "Bake the current map's acoustic probes in the background, replacing its cached bake")
{
	s_neoHrtfSystem.StartBake();
}

CON_COMMAND(cl_neo_hrtf_bake_cancel, "Stop baking the current map's acoustic probes")
{
	s_neoHrtfSystem.CancelBake();
}

void CNeoHrtfSystem::Shutdown()
{
	StopDevice();
	m_cache.PurgeAndDeleteElements();
	m_soundLevels.Purge();
}

void CNeoHrtfSystem::LevelInitPreEntity()
{
	// Built while the map loads when HRTF is already on; otherwise on the first in-game Update
	// after it is enabled, so players without HRTF never read the BSP.
	m_bSceneStale = true;
	if (m_pSpatializer)
	{
		BuildScene();
	}
}

void CNeoHrtfSystem::LevelInitPostEntity()
{
	// Maps can add level_sounds scripts, so the level map is rebuilt against this map's scripts.
	m_soundLevels.Purge();
}

void CNeoHrtfSystem::LevelShutdownPreEntity()
{
	ReleaseAllVoices();
	m_ignoredGuids.RemoveAll();
}

void CNeoHrtfSystem::LevelShutdownPostEntity()
{
	// Cached sounds bake in this map's sound levels, so they do not outlive it.
#ifdef DBGFLAG_ASSERT
	for (const Voice &voice : m_voices)
	{
		Assert(!voice.m_bInUse);
	}
#endif
	m_cache.PurgeAndDeleteElements();
	ReleaseScene();
	m_bSceneStale = false;
}

void CNeoHrtfSystem::Update(float)
{
	if (!cl_neo_hrtf.GetBool())
	{
		// Disabling also clears a failed start, so re-enabling retries it.
		StopDevice();
		m_szStartError[0] = '\0';
		return;
	}

	if (!m_pDevice && !m_szStartError[0])
	{
		StartDevice();
	}

	if (m_pDevice)
	{
		if (engine->IsInGame())
		{
			if (m_bSceneStale)
			{
				BuildScene();
			}
			PollEngineSounds();
			UpdateBake();
			if (cl_neo_hrtf_debug_probes.GetBool())
			{
				DrawProbes();
			}
		}
		else
		{
			ReleaseAllVoices();
		}
	}

	if (cl_neo_hrtf_debug.GetBool())
	{
		PrintDebug();
	}
}

void CNeoHrtfSystem::PollEngineSounds()
{
	static ConVarRef s_masterVolume("volume");
	Assert(s_masterVolume.IsValid());
	// Our device bypasses the engine mixer, so the master volume has to be applied here.
	const float outputScale = cl_neo_hrtf_volume.GetFloat() * s_masterVolume.GetFloat();

	const float engineFloor = cl_neo_hrtf_engine_floor.GetFloat();

	m_listenerOrigin = MainViewOrigin();
	AngleVectors(MainViewAngles(), &m_listenerForward, &m_listenerRight, &m_listenerUp);

	m_activeSounds.RemoveAll();
	enginesound->GetActiveSounds(m_activeSounds);

	int liveVoices = 0;
	for (Voice &voice : m_voices)
	{
		voice.m_bSeenThisPoll = false;
		liveVoices += voice.m_bInUse ? 1 : 0;
	}

	// Classify every engine channel. Existing voices only get new parameters; string work and
	// decoding happen once per guid, outside the lock.
	m_ignoredGuidsNext.RemoveAll();
	int pendingCount = 0;
	const int localPlayerIndex = engine->GetLocalPlayer();
	for (int i = 0; i < m_activeSounds.Count(); ++i)
	{
		const SndInfo_t &info = m_activeSounds[i];
		const int voiceIndex = FindVoice(info.m_nGuid);
		if (voiceIndex >= 0)
		{
			Voice &voice = m_voices[voiceIndex];
			voice.m_bSeenThisPoll = true;
			MuteEngineCopy(voice, info, engineFloor);
			m_stagedParams[voiceIndex] = ComputeParams(info, voice.m_sourceVolume, voice.m_distMult, outputScale);
			continue;
		}

		if (m_ignoredGuids.HasElement(info.m_nGuid))
		{
			m_ignoredGuidsNext.AddToTail(info.m_nGuid);
			continue;
		}

		if (!HrtfIsSpatialCandidate(info, localPlayerIndex))
		{
			continue;
		}

		const bool bHasFreeVoice = liveVoices + pendingCount < kMaxVoices;
		const CachedSound *pSound = bHasFreeVoice ? FindOrLoadSound(info) : nullptr;
		if (!pSound)
		{
			m_ignoredGuidsNext.AddToTail(info.m_nGuid);
			continue;
		}
		m_pending[pendingCount++] = { i, pSound, -1 };
	}

	{
		AUTO_LOCK(m_mutex);
		m_listener = { HrtfToVec3(m_listenerOrigin, kHrtfMetresPerUnit), HrtfToVec3(m_listenerForward),
					   HrtfToVec3(m_listenerRight), HrtfToVec3(m_listenerUp) };

		for (int v = 0; v < kMaxVoices; ++v)
		{
			Voice &voice = m_voices[v];
			if (!voice.m_bInUse)
			{
				continue;
			}
			if (voice.m_bSeenThisPoll)
			{
				voice.m_params = m_stagedParams[v];
			}
			else
			{
				// The engine copy is held above its cull level, so a channel that is gone has
				// finished or been stopped by the game.
				m_pSpatializer->ReleaseVoice(voice.m_hSpatial);
				voice = Voice();
			}
		}

		int freeSearch = 0;
		for (int p = 0; p < pendingCount; ++p)
		{
			PendingVoice &pending = m_pending[p];
			while (m_voices[freeSearch].m_bInUse)
			{
				++freeSearch;
				Assert(freeSearch < kMaxVoices);
			}

			const NeoSpatial::VoiceHandle hSpatial = m_pSpatializer->CreateVoice();
			if (hSpatial == NeoSpatial::INVALID_VOICE)
			{
				continue;
			}

			const SndInfo_t &info = m_activeSounds[pending.m_soundIndex];
			Voice &voice = m_voices[freeSearch];
			voice.m_bInUse = true;
			voice.m_guid = info.m_nGuid;
			voice.m_sourceVolume = info.m_flVolume;
			voice.m_bSeenThisPoll = true;
			voice.m_pSound = pending.m_pSound;
			voice.m_hSpatial = hSpatial;
			ResolveSoundLevel(voice, info, *pending.m_pSound);
			voice.m_params = ComputeParams(info, info.m_flVolume, voice.m_distMult, outputScale);
			pending.m_voiceIndex = freeSearch;
		}
	}

	SimulatePaths();

	// Muting is an engine call, so it stays out of the audio thread's lock.
	for (int p = 0; p < pendingCount; ++p)
	{
		const PendingVoice &pending = m_pending[p];
		const SndInfo_t &info = m_activeSounds[pending.m_soundIndex];
		if (pending.m_voiceIndex >= 0)
		{
			MuteEngineCopy(m_voices[pending.m_voiceIndex], info, engineFloor);
		}
		else
		{
			m_ignoredGuidsNext.AddToTail(info.m_nGuid);
		}
	}
	m_ignoredGuids.Swap(m_ignoredGuidsNext);
}

// Our device bypasses the engine, so its copy of the sound has to be muted, but the engine frees a
// one-shot channel that falls silent. The copy is held at spatializedTarget instead: the lowest
// volume that, times the engine's own gain for the channel, stays at that level.
void CNeoHrtfSystem::MuteEngineCopy(Voice &voice, const SndInfo_t &info, float spatializedTarget)
{
	voice.m_lastReportedVolume = info.m_flVolume;
	voice.m_lastSpatializedVolume = info.m_flLastSpatializedVolume;

	// Once muted the engine reports our level back, not the sound's, so only a report that differs
	// from what was set is a genuine (server) volume change worth remembering.
	const bool bFirstSight = voice.m_engineVolume < 0.0f;
	const bool bReadsBackOurs = !bFirstSight && fabsf(info.m_flVolume - voice.m_engineVolume) <= kHrtfVolumeReadbackTolerance;
	if (!bReadsBackOurs)
	{
		voice.m_sourceVolume = info.m_flVolume;
	}

	// The spatialized volume is the channel volume times the engine's distance and pan gain, but it
	// may have been computed before a volume set last poll, so the gain is only measured while the
	// volume held.
	if (!voice.m_bEngineVolumeJustSet && (bFirstSight || bReadsBackOurs) && info.m_flVolume >= kHrtfEngineVolumeStep
		&& info.m_flLastSpatializedVolume > 0.0f)
	{
		voice.m_engineGain = info.m_flLastSpatializedVolume / info.m_flVolume;
	}

	// The lowest whole step that keeps the spatialized level at the target. A sound whose own
	// volume cannot reach it is left as it is: the engine would cull it unmuted too.
	float target = 0.0f;
	if (spatializedTarget > 0.0f)
	{
		const float gain = (voice.m_engineGain > 0.0f) ? voice.m_engineGain : 1.0f;
		const float steps = Max(ceilf(spatializedTarget / (gain * kHrtfEngineVolumeStep)), 1.0f);
		target = Min(steps * kHrtfEngineVolumeStep, voice.m_sourceVolume);
	}

	const bool bRaise = target > voice.m_engineVolume;
	const bool bLower = target < voice.m_engineVolume * kHrtfMuteLowerRatio || (target == 0.0f && voice.m_engineVolume > 0.0f);
	voice.m_bEngineVolumeJustSet = bFirstSight || !bReadsBackOurs || bRaise || bLower;
	if (voice.m_bEngineVolumeJustSet)
	{
		enginesound->SetVolumeByGuid(info.m_nGuid, target);
		voice.m_engineVolume = target;
	}
}

void CNeoHrtfSystem::SimulatePaths()
{
	// The game thread is the only writer of the voice table, so it can be read without the lock;
	// the simulation itself shares nothing with the audio thread (neo_spatializer.h).
	const double startTime = Plat_FloatTime();
	int count = 0;
	for (int v = 0; v < kMaxVoices; ++v)
	{
		const Voice &voice = m_voices[v];
		if (voice.m_bInUse)
		{
			NeoSpatial::Vec3 origin = voice.m_params.m_origin;
			origin.z += kHrtfOcclusionLiftUnits * kHrtfMetresPerUnit;
			m_simHandles[count] = voice.m_hSpatial;
			m_simOrigins[count] = origin;
			m_simVoiceIndices[count] = v;
			++count;
		}
	}

	if (cl_neo_hrtf_occlusion.GetBool())
	{
		m_pSpatializer->SimulateDirect(m_listener, m_simHandles, m_simOrigins, m_simPaths, count);
	}
	else
	{
		std::fill_n(m_simPaths, count, NeoSpatial::DirectPath());
	}
	// Paths run through the probe batch of the listener's BSP area; a sound in another area (behind
	// an areaportal) has no path to the listener.
	// Steam Audio only recomputes a voice's paths while both it and the listener are inside some
	// probe's sphere of influence; otherwise it hands back the last ones unchanged (measured on
	// ntre_oilstain_ctg: two of its three skylines.wav emitters hang 2-4.5 m from the nearest probe,
	// and their paths froze at the last line-of-sight value). Played with the distance compensation
	// below, a frozen path stayed at one level however far the listener went. So both ends are
	// pulled just inside the nearest probe's sphere when they are outside every sphere, and paths
	// are dropped when no probe is within reach at all.
	std::fill_n(m_simPathing, count, NeoSpatial::PathParams());
	const int listenerLeaf = m_bspTree.leaves.empty() ? -1 : m_bspTree.FindLeaf(m_listener.origin);
	NeoSpatial::Listener pathListener = m_listener;
	if (cl_neo_hrtf_pathing.GetBool() && count > 0 && listenerLeaf >= 0
		&& m_probeCoverage.PullInside(m_listener.origin, kHrtfPathReachMetres, pathListener.origin))
	{
		for (int i = 0; i < count; ++i)
		{
			// Traced from the same raised origin as occlusion, which also keeps floor-level entity
			// sounds within reach of the probes 1.5 m up.
			const Voice &voice = m_voices[m_simVoiceIndices[i]];
			NeoSpatial::Vec3 pathOrigin;
			const bool bReachable = m_probeCoverage.PullInside(m_simOrigins[i], kHrtfPathReachMetres, pathOrigin);
			m_simPathingVoices[i] = { bReachable ? voice.m_hSpatial : NeoSpatial::INVALID_VOICE, pathOrigin,
									  voice.m_params.m_falloffPerMetre };
		}
		m_pSpatializer->SimulatePathing(pathListener, m_bspTree.leaves[listenerLeaf].area, kHrtfEngineGainMin,
										m_simPathingVoices, m_simPathing, count);

		// A path is never shorter than the straight line, so it can never be louder than the voice
		// would be in plain view. Capping at that level means no path can stay audible as the
		// listener walks away, whatever the simulation returned.
		for (int i = 0; i < count; ++i)
		{
			NeoSpatial::PathParams &paths = m_simPathing[i];
			const float maxLevel = kHrtfFirstOrderOmniGain * m_voices[m_simVoiceIndices[i]].m_params.m_distanceGain;
			if (paths.valid && paths.sh[0] > maxLevel)
			{
				const float scale = (paths.sh[0] > 0.0f) ? maxLevel / paths.sh[0] : 0.0f;
				for (float &coeff : paths.sh)
				{
					coeff *= scale;
				}
			}
		}
	}

	// A lookup in the baked probes around the listener, no rays.
	NeoSpatial::ReverbParams reverb;
	const float reverbGain = cl_neo_hrtf_reverb.GetFloat();
	if (reverbGain > 0.0f)
	{
		m_pSpatializer->SimulateReverb(m_listener, &reverb);
	}
	m_simMilliseconds = static_cast<float>((Plat_FloatTime() - startTime) * 1000.0);

	AUTO_LOCK(m_mutex);
	for (int i = 0; i < count; ++i)
	{
		Voice &voice = m_voices[m_simVoiceIndices[i]];
		voice.m_path = m_simPaths[i];
		voice.m_paths = m_simPathing[i];
		voice.m_bHasPath = true;
	}
	m_reverbParams = reverb;
	m_reverbGain = reverbGain;
	m_bReverbDecorrelate = !cl_neo_hrtf_reverb_inhead.GetBool();
}

void CNeoHrtfSystem::StartDevice()
{
	Assert(!m_pDevice && !m_pSpatializer);
	char path[MAX_PATH];
	V_snprintf(path, sizeof(path), "%s/%s", engine->GetGameDirectory(), kHrtfPhononLibrary);
	V_FixSlashes(path);
	m_pSpatializer = NeoSpatial::CreateSteamAudioSpatializer(path, kSampleRate, kFrameSize,
															 m_szStartError, sizeof(m_szStartError));
	if (m_pSpatializer)
	{
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32;
		config.playback.channels = kHrtfOutputChannels;
		config.sampleRate = kSampleRate;
		config.dataCallback = HrtfDataCallback;
		config.pUserData = this;

		// A new spatializer starts without a scene; Update builds it once in game.
		m_bSceneStale = true;
		m_carryRead = 0;
		m_carryAvailable = 0;
		m_pDevice = new ma_device;
		if (ma_device_init(nullptr, &config, m_pDevice) != MA_SUCCESS)
		{
			V_strncpy(m_szStartError, "could not open an audio output device", sizeof(m_szStartError));
			delete m_pDevice;
			m_pDevice = nullptr;
		}
		else if (ma_device_start(m_pDevice) != MA_SUCCESS)
		{
			V_strncpy(m_szStartError, "could not start the audio output device", sizeof(m_szStartError));
		}
	}

	// Every failure leaves a message, which is also what stops Update retrying every frame.
	Assert(m_pSpatializer || m_szStartError[0]);
	if (m_szStartError[0])
	{
		Warning("NEO HRTF: disabled, %s\n", m_szStartError);
		StopDevice();
	}
}

void CNeoHrtfSystem::StopDevice()
{
	if (m_pDevice)
	{
		// Uninit joins the audio thread, so nothing below can race the callback.
		ma_device_uninit(m_pDevice);
		delete m_pDevice;
		m_pDevice = nullptr;
	}
	if (m_pSpatializer)
	{
		ReleaseAllVoices();
		delete m_pSpatializer;
		m_pSpatializer = nullptr;
		m_sceneTriangles = 0;
	}
}

void CNeoHrtfSystem::BuildScene()
{
	Assert(m_pSpatializer);
	m_bSceneStale = false;
	const char *pszMapName = MapName();
	if (!pszMapName || !pszMapName[0])
	{
		return;
	}

	// Scene calls share no state with Process (neo_spatializer.h), so the audio thread keeps
	// rendering while the BSP is read and the scene committed.
	const double startTime = Plat_FloatTime();
	char path[MAX_PATH];
	V_snprintf(path, sizeof(path), "maps/%s.bsp", pszMapName);
	char error[kMaxErrorLen];
	CNeoAudioGeometry geometry;
	const CNeoAudioGeometry::Services services = { filesystem, physprops, physcollision };
	if (!geometry.LoadFromBsp(services, path, error, sizeof(error))
		|| !m_pSpatializer->SetSceneGeometry(geometry.GetSceneGeometry(), error, sizeof(error)))
	{
		// Without a scene sounds are simply unoccluded, as they were before scenes existed.
		Warning("NEO HRTF: no acoustic scene for %s, %s\n", pszMapName, error);
		ReleaseScene();
		return;
	}

	m_sceneTriangles = geometry.NumTriangles();
	DevMsg("NEO HRTF: acoustic scene for %s, %d triangles, built in %.0f ms\n", pszMapName, m_sceneTriangles,
		   (Plat_FloatTime() - startTime) * 1000.0);

	V_snprintf(m_szProbeCachePath, sizeof(m_szProbeCachePath), "%s/%s.probes", kHrtfProbeCacheDir, pszMapName);
	SetupProbes(geometry);
}

void CNeoHrtfSystem::ReleaseScene()
{
	// The backend drops probe batches (cancelling any bake) together with the scene.
	if (m_pSpatializer)
	{
		char error[kMaxErrorLen];
		m_pSpatializer->SetSceneGeometry(NeoSpatial::SceneGeometry(), error, sizeof(error));
	}
	m_sceneTriangles = 0;
	{
		// The room was this map's.
		AUTO_LOCK(m_mutex);
		m_reverbParams = NeoSpatial::ReverbParams();
	}
	m_probes = NeoSpatial::ProbeSet();
	m_bspTree = NeoSpatial::BspTree();
	m_probeCoverage = NeoSpatial::ProbeCoverage();
	m_probeCacheKey = 0;
	m_szProbeCachePath[0] = '\0';
	m_lastBakeState = NeoSpatial::BakeState::Idle;
}

void CNeoHrtfSystem::SetupProbes(const CNeoAudioGeometry &geometry)
{
	m_bspTree = geometry.GetBspTree();

	// A map compiled with utils/neo_soundbake carries its probes baked; nothing to do at runtime.
	if (LoadBakedProbeLump(geometry))
	{
		return;
	}

	// Generation is quick and deterministic, so it always runs: the cache only has to hold the
	// baked data, and the probes are known here for the debug view either way.
	const double startTime = Plat_FloatTime();
	NeoSpatial::BuildLeafProbes(*m_pSpatializer, geometry.GetBspTree(), NeoSpatial::DEFAULT_PROBE_SETTINGS, m_probes);
	m_probeCoverage.Build(m_probes);
	const double generateMs = (Plat_FloatTime() - startTime) * 1000.0;

	CRC32_t key;
	CRC32_Init(&key);
	const uint32 geometryCrc = geometry.GetGeometryCrc();
	CRC32_ProcessBuffer(&key, &geometryCrc, sizeof(geometryCrc));
	CRC32_ProcessBuffer(&key, &kHrtfProbeCacheVersion, sizeof(kHrtfProbeCacheVersion));
	CRC32_ProcessBuffer(&key, m_probes.centres.data(), static_cast<int>(m_probes.centres.size() * sizeof(NeoSpatial::Vec3)));
	CRC32_ProcessBuffer(&key, m_probes.batchStarts.data(), static_cast<int>(m_probes.batchStarts.size() * sizeof(int32_t)));
	CRC32_ProcessBuffer(&key, m_probes.batchAreas.data(), static_cast<int>(m_probes.batchAreas.size() * sizeof(int32_t)));
	CRC32_ProcessBuffer(&key, &m_probes.radius, sizeof(m_probes.radius));
	CRC32_Final(&key);
	m_probeCacheKey = key;

	DevMsg("NEO HRTF: %d acoustic probes in %d batches, generated in %.0f ms\n",
		   static_cast<int>(m_probes.centres.size()), m_probes.NumBatches(), generateMs);
	if (m_probes.centres.empty() || LoadProbeCache())
	{
		return;
	}

	char error[kMaxErrorLen];
	if (!m_pSpatializer->SetProbeBatches(m_probes.Layout(), error, sizeof(error)))
	{
		Warning("NEO HRTF: no acoustic probes, %s\n", error);
		return;
	}
	if (cl_neo_hrtf_bake_auto.GetBool())
	{
		StartBake();
	}
}

bool CNeoHrtfSystem::LoadBakedProbeLump(const CNeoAudioGeometry &geometry)
{
	char error[kMaxErrorLen];
	NeoSpatial::ProbeLumpHeader header;
	NeoSpatial::ProbeSet probes;
	CUtlVector<uint8> storage;
	const uint8 *pBatches = nullptr;
	if (!geometry.ReadBakedProbes(header, probes, storage, pBatches, error, sizeof(error)))
	{
		// No message for a map simply compiled without the baker.
		if (error[0])
		{
			Warning("NEO HRTF: %s; baking at runtime\n", error);
		}
		return false;
	}

	if (!m_pSpatializer->LoadProbeBatches(pBatches, header.batchesSize, error, sizeof(error))
		|| m_pSpatializer->GetBakeState(nullptr) != NeoSpatial::BakeState::Done)
	{
		// E.g. baked with a Steam Audio whose format this one cannot read.
		DevWarning("NEO HRTF: cannot use the map's baked probes (%s, baked with Steam Audio %u.%u.%u); baking at runtime\n",
				   error[0] ? error : "incomplete bake", (header.libraryVersion >> 16) & 0xff, (header.libraryVersion >> 8) & 0xff,
				   header.libraryVersion & 0xff);
		return false;
	}

	m_probes = std::move(probes);
	m_probeCoverage.Build(m_probes);
	m_lastBakeState = NeoSpatial::BakeState::Done;
	DevMsg("NEO HRTF: loaded %d acoustic probes in %d batches baked into the map\n", static_cast<int>(m_probes.centres.size()),
		   m_probes.NumBatches());
	return true;
}

bool CNeoHrtfSystem::LoadProbeCache()
{
	CUtlBuffer file;
	if (!filesystem->ReadFile(m_szProbeCachePath, "MOD", file))
	{
		return false;
	}

	HrtfProbeCacheHeader header;
	if (file.TellPut() < static_cast<int>(sizeof(header)))
	{
		return false;
	}
	V_memcpy(&header, file.Base(), sizeof(header));
	if (header.magic != kHrtfProbeCacheMagic || header.version != kHrtfProbeCacheVersion || header.key != m_probeCacheKey)
	{
		DevMsg("NEO HRTF: %s is for a different map build or bake version, rebaking\n", m_szProbeCachePath);
		return false;
	}

	char error[kMaxErrorLen];
	const uint8 *pBlob = static_cast<const uint8 *>(file.Base()) + sizeof(header);
	if (!m_pSpatializer->LoadProbeBatches(pBlob, file.TellPut() - sizeof(header), error, sizeof(error))
		|| m_pSpatializer->GetBakeState(nullptr) != NeoSpatial::BakeState::Done)
	{
		DevWarning("NEO HRTF: cannot use %s (%s), rebaking\n", m_szProbeCachePath, error[0] ? error : "incomplete bake");
		return false;
	}
	m_lastBakeState = NeoSpatial::BakeState::Done;
	DevMsg("NEO HRTF: loaded baked probes from %s\n", m_szProbeCachePath);
	return true;
}

void CNeoHrtfSystem::StartBake()
{
	if (!m_pSpatializer || m_probes.centres.empty())
	{
		Msg("NEO HRTF: no acoustic probes to bake (needs cl_neo_hrtf 1 and a loaded map)\n");
		return;
	}

	// A cached bake may be loaded, so the batches are rebuilt from the generated probes first.
	char error[kMaxErrorLen];
	if (m_pSpatializer->GetBakeState(nullptr) != NeoSpatial::BakeState::Idle
		&& !m_pSpatializer->SetProbeBatches(m_probes.Layout(), error, sizeof(error)))
	{
		Warning("NEO HRTF: cannot bake, %s\n", error);
		return;
	}

	int threads = cl_neo_hrtf_bake_threads.GetInt();
	if (threads <= 0)
	{
		threads = Max(1, GetCPUInformation()->m_nLogicalProcessors / kHrtfBakeCoresPerThread);
	}
	if (m_pSpatializer->StartBake(threads))
	{
		m_lastBakeState = NeoSpatial::BakeState::Running;
		DevMsg("NEO HRTF: baking %d acoustic probes on %d threads\n", static_cast<int>(m_probes.centres.size()), threads);
	}
}

void CNeoHrtfSystem::CancelBake()
{
	if (m_pSpatializer)
	{
		m_pSpatializer->CancelBake();
	}
}

void CNeoHrtfSystem::UpdateBake()
{
	const NeoSpatial::BakeState state = m_pSpatializer->GetBakeState(nullptr);
	if (state == m_lastBakeState)
	{
		return;
	}
	m_lastBakeState = state;
	if (state == NeoSpatial::BakeState::Done)
	{
		SaveProbeCache();
	}
	else if (state == NeoSpatial::BakeState::Failed)
	{
		DevMsg("NEO HRTF: acoustic probe bake stopped before finishing; cl_neo_hrtf_bake restarts it\n");
	}
}

void CNeoHrtfSystem::SaveProbeCache()
{
	int64_t size = 0;
	const uint8_t *pBlob = m_pSpatializer->SerializeProbeBatches(&size);
	if (!pBlob || !m_szProbeCachePath[0])
	{
		return;
	}

	const HrtfProbeCacheHeader header = { kHrtfProbeCacheMagic, kHrtfProbeCacheVersion, m_probeCacheKey, 0 };
	CUtlBuffer file;
	file.Put(&header, sizeof(header));
	file.Put(pBlob, static_cast<int>(size));
	filesystem->CreateDirHierarchy(kHrtfProbeCacheDir, "MOD");
	if (filesystem->WriteFile(m_szProbeCachePath, "MOD", file))
	{
		DevMsg("NEO HRTF: baked acoustic probes saved to %s (%lld bytes)\n", m_szProbeCachePath, static_cast<long long>(size));
	}
	else
	{
		Warning("NEO HRTF: could not write %s\n", m_szProbeCachePath);
	}
}

void CNeoHrtfSystem::DrawProbes() const
{
	const Vector boxMins(-kHrtfProbeDrawSizeUnits, -kHrtfProbeDrawSizeUnits, -kHrtfProbeDrawSizeUnits);
	const Vector boxMaxs = -boxMins;
	for (int b = 0; b < m_probes.NumBatches(); ++b)
	{
		// Neighbouring batches get visibly different colours.
		const uint32 hash = static_cast<uint32>(m_probes.batchAreas[b]) * 2654435761u;
		const int r = 128 + (hash & 127), g = 128 + ((hash >> 8) & 127), blue = (hash >> 16) & 255;
		for (int p = m_probes.batchStarts[b]; p < m_probes.batchStarts[b + 1]; ++p)
		{
			const NeoSpatial::Vec3 &centre = m_probes.centres[p];
			const Vector position(centre.x, centre.y, centre.z);
			const Vector origin = position / kHrtfMetresPerUnit;
			if (origin.DistToSqr(m_listenerOrigin) < kHrtfProbeDrawRangeUnits * kHrtfProbeDrawRangeUnits)
			{
				debugoverlay->AddBoxOverlay(origin, boxMins, boxMaxs, vec3_angle, r, g, blue, 160, 0.0f);
			}
		}
	}
}

void CNeoHrtfSystem::SaveSceneObj() const
{
	const char *pszMapName = MapName();
	if (!m_pSpatializer || !pszMapName)
	{
		Msg("NEO HRTF: no acoustic scene (needs cl_neo_hrtf 1 and a loaded map)\n");
		return;
	}

	char fileBaseName[MAX_PATH];
	V_snprintf(fileBaseName, sizeof(fileBaseName), "%s/hrtf_scene_%s", engine->GetGameDirectory(), pszMapName);
	V_FixSlashes(fileBaseName);
	if (m_pSpatializer->SaveSceneObj(fileBaseName))
	{
		Msg("NEO HRTF: wrote %s.obj\n", fileBaseName);
	}
	else
	{
		Msg("NEO HRTF: no acoustic scene for %s\n", pszMapName);
	}
}

void CNeoHrtfSystem::ReleaseAllVoices()
{
	// Hand still-playing sounds back to the engine; a no-op for guids that already ended.
	for (const Voice &voice : m_voices)
	{
		if (voice.m_bInUse && enginesound->IsSoundStillPlaying(voice.m_guid))
		{
			enginesound->SetVolumeByGuid(voice.m_guid, voice.m_sourceVolume);
		}
	}

	AUTO_LOCK(m_mutex);
	for (Voice &voice : m_voices)
	{
		if (voice.m_bInUse)
		{
			m_pSpatializer->ReleaseVoice(voice.m_hSpatial);
			voice = Voice();
		}
	}
}

const CNeoHrtfSystem::CachedSound *CNeoHrtfSystem::FindOrLoadSound(const SndInfo_t &info)
{
	char rawName[MAX_PATH];
	if (!filesystem->String(info.m_filenameHandle, rawName, sizeof(rawName)) || HrtfIsEngineOnlyName(rawName))
	{
		return nullptr;
	}

	char name[MAX_PATH];
	HrtfNormaliseSoundName(rawName, name, sizeof(name));
	const char *pszExt = V_GetFileExtension(name);
	if (!pszExt || (V_strcmp(pszExt, "wav") != 0 && V_strcmp(pszExt, "mp3") != 0))
	{
		return nullptr;
	}

	int index = m_cache.Find(name);
	if (index == m_cache.InvalidIndex())
	{
		// Failures are cached too (empty samples), so a bad file is only read once per level.
		CachedSound *pSound = new CachedSound;
		pSound->m_name = name;
		LoadSound(*pSound);
		index = m_cache.Insert(name, pSound);
	}

	const CachedSound *pSound = m_cache[index];
	return pSound->m_samples.IsEmpty() ? nullptr : pSound;
}

void CNeoHrtfSystem::LoadSound(CachedSound &sound)
{
	char path[MAX_PATH];
	V_snprintf(path, sizeof(path), "%s%s", kHrtfSoundDir, sound.m_name.Get());
	CUtlBuffer file;
	ma_decoder decoder;
	const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, kSampleRate);
	if (!filesystem->ReadFile(path, "GAME", file) || file.TellPut() <= 0
		|| ma_decoder_init_memory(file.Base(), file.TellPut(), &config, &decoder) != MA_SUCCESS)
	{
		DevMsg("NEO HRTF: cannot read or decode %s, leaving it to the engine\n", path);
		return;
	}

	ma_uint64 length = 0;
	if (ma_decoder_get_length_in_pcm_frames(&decoder, &length) == MA_SUCCESS
		&& length > 0 && length <= static_cast<ma_uint64>(kHrtfMaxCachedFrames))
	{
		sound.m_samples.SetCount(static_cast<int>(length));
		ma_uint64 framesRead = 0;
		ma_decoder_read_pcm_frames(&decoder, sound.m_samples.Base(), length, &framesRead);
		sound.m_samples.SetCountNonDestructively(static_cast<int>(framesRead));
	}
	else
	{
		DevMsg("NEO HRTF: %s is empty or longer than %d s, leaving it to the engine\n", path, kHrtfMaxCachedSeconds);
	}
	ma_decoder_uninit(&decoder);

	if (V_strcmp(V_GetFileExtension(path), "wav") == 0)
	{
		int loopStart;
		int loopEnd;
		HrtfParseWavLoop(static_cast<const uint8 *>(file.Base()), file.TellPut(), loopStart, loopEnd);
		sound.m_loopStart = (loopStart < sound.m_samples.Count()) ? loopStart : -1;
		// Nothing past the loop end is ever heard while looping, so drop it rather than teach playback a second end.
		if (sound.m_loopStart >= 0 && loopEnd > sound.m_loopStart && loopEnd < sound.m_samples.Count())
		{
			sound.m_samples.SetCountNonDestructively(loopEnd);
		}
	}
	sound.m_scriptLevel = LookupScriptLevel(sound.m_name.Get());
}

void CNeoHrtfSystem::ResolveSoundLevel(Voice &voice, const SndInfo_t &info, const CachedSound &sound) const
{
	// An ambient_generic playing a raw file emits it at the level its radius gives, which no script
	// knows about; the server networks it with the entity index the channel is emitted from. Several
	// emitters can share a source entity (SourceEntityName), so the file has to match as well.
	voice.m_soundLevel = sound.m_scriptLevel;
	voice.m_bLevelFromEmitter = false;
	char name[MAX_PATH];
	for (int i = 0; i < C_AmbientGeneric::Count(); ++i)
	{
		const C_AmbientGeneric *pAmbient = C_AmbientGeneric::Get(i);
		if (pAmbient->GetEmitterIndex() != info.m_nSoundSource || pAmbient->GetNetSoundLevel() < 0)
		{
			continue;
		}
		HrtfNormaliseSoundName(pAmbient->GetSoundFile(), name, sizeof(name));
		if (V_strcmp(name, sound.m_name.Get()) == 0)
		{
			voice.m_soundLevel = static_cast<soundlevel_t>(pAmbient->GetNetSoundLevel());
			voice.m_bLevelFromEmitter = true;
			break;
		}
	}
	voice.m_distMult = HrtfDistMultForLevel(voice.m_soundLevel);
}

soundlevel_t CNeoHrtfSystem::LookupScriptLevel(const char *pszNormalisedName)
{
	// Built on first use rather than at level load, so players without HRTF never pay for it.
	// Channels only carry the wave file, so map every scripted wave back to its loudest level.
	if (m_soundLevels.Count() == 0)
	{
		Assert(soundemitterbase);
		char name[MAX_PATH];
		for (int i = soundemitterbase->First(); i != soundemitterbase->InvalidIndex(); i = soundemitterbase->Next(i))
		{
			const CSoundParametersInternal *pParams = soundemitterbase->InternalGetParametersForSound(i);
			if (!pParams)
			{
				continue;
			}
			const soundlevel_t level = static_cast<soundlevel_t>(pParams->GetSoundLevel().start);
			for (int w = 0; w < pParams->NumSoundNames(); ++w)
			{
				CUtlSymbol waveSymbol = pParams->GetSoundNames()[w].symbol; // GetWaveName takes a non-const ref
				HrtfNormaliseSoundName(soundemitterbase->GetWaveName(waveSymbol), name, sizeof(name));
				const int index = m_soundLevels.Find(name);
				if (index == m_soundLevels.InvalidIndex())
				{
					m_soundLevels.Insert(name, level);
				}
				else
				{
					m_soundLevels[index] = Max(m_soundLevels[index], level);
				}
			}
		}
	}

	const int index = m_soundLevels.Find(pszNormalisedName);
	return (index != m_soundLevels.InvalidIndex()) ? m_soundLevels[index] : SNDLVL_NORM;
}

CNeoHrtfSystem::VoiceParams CNeoHrtfSystem::ComputeParams(const SndInfo_t &info, float sourceVolume, float distMult,
														  float outputScale) const
{
	// The engine refreshes this every frame for channels that follow their entity
	// (GetSoundSpatialization), so it already tracks moving players.
	Assert(info.m_pOrigin);
	const Vector &position = *info.m_pOrigin;

	// The engine's inverse-distance model (see kHrtfEngineRefDb), so audible ranges match
	// the engine's own copy. Below snd_gain_min the engine stops mixing the channel.
	const float relativeDistance = position.DistTo(m_listenerOrigin) * distMult;
	float distanceGain = (relativeDistance > 1.0f) ? (1.0f / relativeDistance) : kHrtfEngineGainMax;
	if (distanceGain < kHrtfEngineGainMin)
	{
		distanceGain = 0.0f;
	}

	Assert(info.m_nPitch > 0);
	// The same law per metre, for the pathing simulation to evaluate along paths around obstacles.
	const float falloffPerMetre = distMult / kHrtfMetresPerUnit;
	return { HrtfToVec3(position, kHrtfMetresPerUnit), sourceVolume * distanceGain * outputScale,
			 static_cast<float>(Max(info.m_nPitch, 1)) / PITCH_NORM, distanceGain, falloffPerMetre };
}

int CNeoHrtfSystem::FindVoice(int guid) const
{
	for (int v = 0; v < kMaxVoices; ++v)
	{
		if (m_voices[v].m_bInUse && m_voices[v].m_guid == guid)
		{
			return v;
		}
	}
	return -1;
}

void CNeoHrtfSystem::PrintDebug() const
{
	if (!m_pDevice)
	{
		engine->Con_NPrintf(0, "hrtf: disabled, %s", m_szStartError);
		return;
	}

	int liveVoices = 0;
	const Vector listenerMetres = m_listenerOrigin * kHrtfMetresPerUnit;
	for (int v = 0; v < kMaxVoices; ++v)
	{
		const Voice &voice = m_voices[v];
		if (!voice.m_bInUse)
		{
			continue;
		}
		++liveVoices;
		const NeoSpatial::Vec3 &origin = voice.m_params.m_origin;
		const Vector toSource = Vector(origin.x, origin.y, origin.z) - listenerMetres;
		const float azimuthDeg = RAD2DEG(atan2f(DotProduct(toSource, m_listenerRight), DotProduct(toSource, m_listenerForward)));
		const NeoSpatial::DirectPath &path = voice.m_path;
		// The paths' omnidirectional component is their overall level along the path.
		char pathText[32] = "paths -";
		if (voice.m_paths.valid)
		{
			V_snprintf(pathText, sizeof(pathText), "paths %.2f", voice.m_paths.sh[0]);
		}
		// "map" when the level came from the networked ambient_generic, "script" otherwise.
		engine->Con_NPrintf(v + 1, "hrtf %s  %.1f m  %d dB %s  gain %.2f  az %+.0f  occ %.2f  trans %.2f/%.2f/%.2f  %s  engine vol %.4f spat %.4f gain %.3f",
			voice.m_pSound->m_name.Get(), toSource.Length(), static_cast<int>(voice.m_soundLevel),
			voice.m_bLevelFromEmitter ? "map" : "script", voice.m_params.m_gain, azimuthDeg, path.occlusion,
			path.transmission[0], path.transmission[1], path.transmission[2], pathText, voice.m_lastReportedVolume,
			voice.m_lastSpatializedVolume, voice.m_engineGain);
	}
	engine->Con_NPrintf(0, "hrtf: Steam Audio, voices %d/%d, cached sounds %d, scripted wave levels %d, scene triangles %d, sim %.2f ms",
		liveVoices, kMaxVoices, m_cache.Count(), m_soundLevels.Count(), m_sceneTriangles, m_simMilliseconds);

	static const char *const s_pszBakeStates[] = { "not baked", "baking", "baked", "bake stopped" };
	float bakeProgress = 0.0f;
	const NeoSpatial::BakeState bakeState = m_pSpatializer->GetBakeState(&bakeProgress);
	engine->Con_NPrintf(kMaxVoices + 1, "hrtf probes: %d in %d batches, %s %.0f%%", static_cast<int>(m_probes.centres.size()),
		m_probes.NumBatches(), s_pszBakeStates[static_cast<int>(bakeState)], bakeProgress * 100.0f);
	if (m_reverbParams.valid)
	{
		engine->Con_NPrintf(kMaxVoices + 2, "hrtf reverb: RT60 %.2f / %.2f / %.2f s (low / mid / high), level %.2f",
			m_reverbParams.reverbTimes[0], m_reverbParams.reverbTimes[1], m_reverbParams.reverbTimes[2], m_reverbGain);
	}
	else
	{
		engine->Con_NPrintf(kMaxVoices + 2, "hrtf reverb: none (probes not baked, or not near one yet)");
	}
}

void CNeoHrtfSystem::Render(float *pOutInterleaved, int frameCount)
{
	// Backends take a fixed block size, while miniaudio asks for whatever its period is,
	// so render whole blocks and hand them out through the carry buffer.
	while (frameCount > 0)
	{
		if (m_carryAvailable == 0)
		{
			RenderBlock();
			m_carryRead = 0;
			m_carryAvailable = kFrameSize;
		}
		const int frames = Min(frameCount, m_carryAvailable);
		V_memcpy(pOutInterleaved, m_carry + m_carryRead * kHrtfOutputChannels, frames * kHrtfOutputChannels * sizeof(float));
		pOutInterleaved += frames * kHrtfOutputChannels;
		frameCount -= frames;
		m_carryRead += frames;
		m_carryAvailable -= frames;
	}
}

void CNeoHrtfSystem::RenderBlock()
{
	V_memset(m_carry, 0, sizeof(m_carry));
	{
		AUTO_LOCK(m_mutex);
		m_pSpatializer->SetListener(m_listener);
		for (Voice &voice : m_voices)
		{
			if (!voice.m_bInUse || voice.m_bFinished || !voice.m_bHasPath)
			{
				continue;
			}
			voice.m_bFinished = !ReadVoiceSamples(voice);

			// Process even inaudible voices: skipping would freeze the backend's filter
			// history and click when the voice becomes audible again.
			// The voice's own gain is also its reverb send, so a far sound excites the room as
			// faintly as it is heard directly.
			const float gain = voice.m_params.m_gain;
			NeoSpatial::VoiceRender render;
			render.direct = voice.m_path;
			render.paths = voice.m_paths;
			render.reverbSend = gain;
			// Paths carry the distance law along their own length, so the straight-line part of
			// gain (applied to the whole output below) is divided back out of them.
			const float distanceGain = voice.m_params.m_distanceGain;
			render.pathGain = (distanceGain > 0.0f) ? 1.0f / distanceGain : 0.0f;
			m_pSpatializer->Process(voice.m_hSpatial, voice.m_params.m_origin, render, m_scratchMono, m_scratchLeft,
									m_scratchRight);
			if (gain <= 0.0f)
			{
				continue;
			}
			for (int f = 0; f < kFrameSize; ++f)
			{
				m_carry[f * kHrtfOutputChannels] += gain * m_scratchLeft[f];
				m_carry[f * kHrtfOutputChannels + 1] += gain * m_scratchRight[f];
			}
		}

		// Every block, voices or not, so tails ring out after the sounds that started them.
		m_pSpatializer->ProcessReverb(m_reverbParams, m_bReverbDecorrelate, m_scratchLeft, m_scratchRight);
		if (m_reverbParams.valid && m_reverbGain > 0.0f)
		{
			for (int f = 0; f < kFrameSize; ++f)
			{
				m_carry[f * kHrtfOutputChannels] += m_reverbGain * m_scratchLeft[f];
				m_carry[f * kHrtfOutputChannels + 1] += m_reverbGain * m_scratchRight[f];
			}
		}
	}

	for (float &sample : m_carry)
	{
		sample = clamp(sample, -1.0f, 1.0f);
	}
}

bool CNeoHrtfSystem::ReadVoiceSamples(Voice &voice)
{
	const CachedSound &sound = *voice.m_pSound;
	const float *pSamples = sound.m_samples.Base();
	const int count = sound.m_samples.Count();
	const int loopStart = sound.m_loopStart;
	const double rate = voice.m_params.m_rate;
	double cursor = voice.m_cursor;

	for (int f = 0; f < kFrameSize; ++f)
	{
		if (cursor >= count)
		{
			if (loopStart < 0)
			{
				V_memset(m_scratchMono + f, 0, (kFrameSize - f) * sizeof(float));
				voice.m_cursor = cursor;
				return false;
			}
			cursor = loopStart + fmod(cursor - loopStart, static_cast<double>(count - loopStart));
		}

		// Linear interpolation is enough for engine pitch shifts, which stay within an octave or so.
		const int index = static_cast<int>(cursor);
		int nextIndex = index + 1;
		if (nextIndex >= count)
		{
			nextIndex = (loopStart >= 0) ? loopStart : index;
		}
		const float frac = static_cast<float>(cursor - index);
		m_scratchMono[f] = pSamples[index] + (pSamples[nextIndex] - pSamples[index]) * frac;
		cursor += rate;
	}
	voice.m_cursor = cursor;
	return true;
}
