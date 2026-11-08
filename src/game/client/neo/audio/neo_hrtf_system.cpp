#include "cbase.h"
#include "neo_hrtf_system.h"

#include "engine/IEngineSound.h"
#include "filesystem.h"
#include "soundchars.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "utlbuffer.h"
#include "view.h"

#include "miniaudio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

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

uint32 HrtfReadLE32(const uint8 *pData)
{
	return uint32(pData[0]) | (uint32(pData[1]) << 8) | (uint32(pData[2]) << 16) | (uint32(pData[3]) << 24);
}

// Source loops a wav from the sample offset of its first cue point (CAudioSourceWave::
// ParseCueChunk), so do the same. Returns the loop start in output-rate frames, or -1.
int HrtfParseWavLoopStart(const uint8 *pData, int size)
{
	if (size < kRiffHeaderSize || V_memcmp(pData, "RIFF", 4) != 0 || V_memcmp(pData + 8, "WAVE", 4) != 0)
	{
		return -1;
	}

	uint32 sourceRate = 0;
	int64 cueSampleOffset = -1;
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
			cueSampleOffset = HrtfReadLE32(pBody + kWavCueCountSize + kWavCueSampleOffsetOffset);
		}
		pos += kRiffChunkHeaderSize + chunkSize + (chunkSize & 1);
	}

	if (sourceRate == 0 || cueSampleOffset < 0)
	{
		return -1;
	}
	return static_cast<int>(cueSampleOffset * CNeoHrtfSystem::kSampleRate / sourceRate);
}

// Soundscripts and engine channels both reduce to this form: sound chars stripped,
// lower case, forward slashes, relative to sound/.
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

// Once the engine copy is muted its reported volume is our 0, not the sound's, so only a
// non-zero report is a genuine (server) volume change worth remembering.
void HrtfMuteEngineCopy(float &sourceVolume, const SndInfo_t &info)
{
	if (info.m_flVolume > 0.0f)
	{
		sourceVolume = info.m_flVolume;
		enginesound->SetVolumeByGuid(info.m_nGuid, 0.0f);
	}
}

void HrtfDataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	(void)pInput;
	static_cast<CNeoHrtfSystem *>(pDevice->pUserData)->Render(static_cast<float *>(pOutput), static_cast<int>(frameCount));
}

} // namespace

// NEO HRTF: cheat-protected and not archived while the feature is under development, so only
// developers and testers on an sv_cheats server can turn it on, and it is off on every start.
ConVar cl_neo_hrtf("cl_neo_hrtf", "0", FCVAR_CLIENTDLL | FCVAR_CHEAT,
	"Re-render positional sounds with HRTF (in development)", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_volume("cl_neo_hrtf_volume", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Volume of the HRTF output, on top of the master volume", true, 0.0f, true, 1.0f);
ConVar cl_neo_hrtf_debug("cl_neo_hrtf_debug", "0", FCVAR_CLIENTDLL,
	"Overlay the HRTF status and one line per voice: file, distance, gain, azimuth", true, 0.0f, true, 1.0f);

static CNeoHrtfSystem s_neoHrtfSystem;

void CNeoHrtfSystem::Shutdown()
{
	StopDevice();
	m_cache.PurgeAndDeleteElements();
	m_soundLevels.Purge();
}

void CNeoHrtfSystem::LevelInitPostEntity()
{
	// Maps can add level_sounds scripts, so the level map is rebuilt against this map's scripts.
	m_soundLevels.Purge();
}

void CNeoHrtfSystem::LevelShutdownPreEntity()
{
	if (m_pSpatializer)
	{
		ReleaseAllVoices();
	}
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
}

void CNeoHrtfSystem::Update(float)
{
	if (!cl_neo_hrtf.GetBool())
	{
		// The disabled path is one cvar read: nothing was started, so there is nothing to undo.
		// Disabling after a run (including sv_cheats reverting the cvar) tears down once here
		// and hands any muted sounds back to the engine; it also clears a failed start.
		if (m_pDevice || m_pSpatializer || m_szStartError[0])
		{
			StopDevice();
			m_szStartError[0] = '\0';
		}
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
			PollEngineSounds();
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

	Vector listenerUp;
	m_listenerOrigin = MainViewOrigin();
	AngleVectors(MainViewAngles(), &m_listenerForward, &m_listenerRight, &listenerUp);

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
			HrtfMuteEngineCopy(voice.m_sourceVolume, info);
			m_stagedParams[voiceIndex] = ComputeParams(info, voice.m_sourceVolume, *voice.m_pSound, outputScale);
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
					   HrtfToVec3(m_listenerRight), HrtfToVec3(listenerUp) };

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
			voice.m_params = ComputeParams(info, info.m_flVolume, *pending.m_pSound, outputScale);
			pending.m_voiceIndex = freeSearch;
		}
	}

	// Muting is an engine call, so it stays out of the audio thread's lock.
	for (int p = 0; p < pendingCount; ++p)
	{
		const PendingVoice &pending = m_pending[p];
		const SndInfo_t &info = m_activeSounds[pending.m_soundIndex];
		if (pending.m_voiceIndex >= 0)
		{
			HrtfMuteEngineCopy(m_voices[pending.m_voiceIndex].m_sourceVolume, info);
		}
		else
		{
			m_ignoredGuidsNext.AddToTail(info.m_nGuid);
		}
	}
	m_ignoredGuids.Swap(m_ignoredGuidsNext);
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
		const int loopStart = HrtfParseWavLoopStart(static_cast<const uint8 *>(file.Base()), file.TellPut());
		sound.m_loopStart = (loopStart < sound.m_samples.Count()) ? loopStart : -1;
	}
	sound.m_distMult = LookupDistMult(sound.m_name.Get());
}

float CNeoHrtfSystem::LookupDistMult(const char *pszNormalisedName)
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
	const soundlevel_t level = (index != m_soundLevels.InvalidIndex()) ? m_soundLevels[index] : SNDLVL_NORM;
	// SNDLVL_NONE means "heard everywhere": no distance falloff at all.
	if (level == SNDLVL_NONE)
	{
		return 0.0f;
	}
	return powf(10.0f, (kHrtfEngineRefDb - static_cast<float>(level)) / 20.0f) / kHrtfEngineRefDistUnits;
}

CNeoHrtfSystem::VoiceParams CNeoHrtfSystem::ComputeParams(const SndInfo_t &info, float sourceVolume,
														  const CachedSound &sound, float outputScale) const
{
	// The engine refreshes this every frame for channels that follow their entity
	// (GetSoundSpatialization), so it already tracks moving players.
	Assert(info.m_pOrigin);
	const Vector &position = *info.m_pOrigin;

	// The engine's inverse-distance model (see kHrtfEngineRefDb), so audible ranges match
	// the engine's own copy. Below snd_gain_min the engine stops mixing the channel.
	const float relativeDistance = position.DistTo(m_listenerOrigin) * sound.m_distMult;
	float distanceGain = (relativeDistance > 1.0f) ? (1.0f / relativeDistance) : kHrtfEngineGainMax;
	if (distanceGain < kHrtfEngineGainMin)
	{
		distanceGain = 0.0f;
	}

	Assert(info.m_nPitch > 0);
	return { HrtfToVec3(position, kHrtfMetresPerUnit), sourceVolume * distanceGain * outputScale,
			 static_cast<float>(Max(info.m_nPitch, 1)) / PITCH_NORM };
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
		engine->Con_NPrintf(v + 1, "hrtf %s  %.1f m  gain %.2f  az %+.0f", voice.m_pSound->m_name.Get(),
			toSource.Length(), voice.m_params.m_gain, azimuthDeg);
	}
	engine->Con_NPrintf(0, "hrtf: Steam Audio, voices %d/%d, cached sounds %d, scripted wave levels %d",
		liveVoices, kMaxVoices, m_cache.Count(), m_soundLevels.Count());
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
			if (!voice.m_bInUse || voice.m_bFinished)
			{
				continue;
			}
			voice.m_bFinished = !ReadVoiceSamples(voice);

			// Process even inaudible voices: skipping would freeze the backend's filter
			// history and click when the voice becomes audible again.
			m_pSpatializer->Process(voice.m_hSpatial, voice.m_params.m_origin, m_scratchMono, m_scratchLeft, m_scratchRight);
			const float gain = voice.m_params.m_gain;
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
