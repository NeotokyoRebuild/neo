// NEO HRTF: client-side re-rendering of the engine's positional sounds through an
// NeoSpatial::ISpatializer backend. The engine mixer is closed source, so instead of
// hooking it this system polls the engine's channel list once per frame, mutes the
// engine's copy of each positional sound and plays its own spatialised copy on a
// separate miniaudio output device.
#pragma once

#include "igamesystem.h"
#include "mathlib/vector.h"
#include "engine/SndInfo.h"
#include "soundflags.h"
#include "tier0/threadtools.h"
#include "utldict.h"
#include "utlstring.h"
#include "utlvector.h"

#include "neo_spatializer.h"

struct ma_device;

class CNeoHrtfSystem : public CAutoGameSystemPerFrame
{
public:
	static constexpr int kSampleRate = 48000;
	static constexpr int kFrameSize = 512;
	static constexpr int kMaxVoices = 32;
	static constexpr int kMaxErrorLen = 256;

	CNeoHrtfSystem() : CAutoGameSystemPerFrame("CNeoHrtfSystem") {}

	void Shutdown() override;
	void LevelInitPostEntity() override;
	void LevelShutdownPreEntity() override;
	void LevelShutdownPostEntity() override;
	void Update(float frametime) override;

	// Audio thread entry point (miniaudio data callback).
	void Render(float *pOutInterleaved, int frameCount);

private:
	// Decoded once per file, immutable afterwards, so the audio thread may read it
	// without locking for as long as a voice references it.
	struct CachedSound
	{
		CUtlString m_name;
		CUtlVector<float> m_samples; // mono, kSampleRate; empty = unplayable, left to the engine
		int m_loopStart = -1; // sample index to loop back to, -1 for one-shots
		float m_distMult = 0.0f; // engine dist_mult of the sound's scripted level, 0 = no falloff
	};

	// Written by the game thread under m_mutex, read by the audio thread under m_mutex.
	struct VoiceParams
	{
		NeoSpatial::Vec3 m_origin; // metres
		float m_gain;
		float m_rate; // playback rate relative to kSampleRate, from the engine pitch
	};

	struct Voice
	{
		// Game thread only.
		bool m_bInUse = false;
		int m_guid = 0;
		float m_sourceVolume = 0.0f; // engine channel volume before we muted it, restored on release
		bool m_bSeenThisPoll = false;

		// Set by the game thread under m_mutex while the voice is created or released.
		const CachedSound *m_pSound = nullptr;
		NeoSpatial::VoiceHandle m_hSpatial = NeoSpatial::INVALID_VOICE;
		VoiceParams m_params = {};

		// Audio thread only (reset under m_mutex when the voice is created).
		double m_cursor = 0.0;
		bool m_bFinished = false;
	};

	struct PendingVoice
	{
		int m_soundIndex; // into m_activeSounds
		const CachedSound *m_pSound;
		int m_voiceIndex; // slot it was given, -1 if the backend refused a voice
	};

	void StartDevice();
	void StopDevice();
	void ReleaseAllVoices();
	void PollEngineSounds();

	const CachedSound *FindOrLoadSound(const SndInfo_t &info);
	void LoadSound(CachedSound &sound);
	float LookupDistMult(const char *pszNormalisedName);

	VoiceParams ComputeParams(const SndInfo_t &info, float sourceVolume, const CachedSound &sound,
							  float outputScale) const;
	int FindVoice(int guid) const;
	void PrintDebug() const;

	void RenderBlock();
	bool ReadVoiceSamples(Voice &voice);

	// Game thread state.
	ma_device *m_pDevice = nullptr;
	NeoSpatial::ISpatializer *m_pSpatializer = nullptr;
	char m_szStartError[kMaxErrorLen] = ""; // why the last start failed; non-empty blocks retries until re-enabled
	CUtlVector<SndInfo_t> m_activeSounds;
	CUtlVector<int> m_ignoredGuids; // sounds deliberately left to the engine, rechecked by guid only
	CUtlVector<int> m_ignoredGuidsNext;
	CUtlDict<CachedSound *, int> m_cache;
	CUtlDict<soundlevel_t, int> m_soundLevels; // normalised wave name → loudest scripted level
	VoiceParams m_stagedParams[kMaxVoices];
	PendingVoice m_pending[kMaxVoices];
	Vector m_listenerOrigin;
	Vector m_listenerForward;
	Vector m_listenerRight;

	// Shared between threads, guarded by m_mutex.
	mutable CThreadMutex m_mutex;
	Voice m_voices[kMaxVoices];
	NeoSpatial::Listener m_listener = {};

	// Audio thread only.
	float m_carry[kFrameSize * 2]; // interleaved stereo block not yet handed to miniaudio
	int m_carryRead = 0;
	int m_carryAvailable = 0;
	float m_scratchMono[kFrameSize];
	float m_scratchLeft[kFrameSize];
	float m_scratchRight[kFrameSize];
};
