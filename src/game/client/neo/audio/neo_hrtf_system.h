// NEO HRTF: client-side re-rendering of the engine's positional sounds through an
// NeoSpatial::ISpatializer backend. The engine mixer is closed source, so instead of
// hooking it this system polls the engine's channel list once per frame, mutes the
// engine's copy of each positional sound and plays its own spatialised copy on a
// separate miniaudio output device.
//
// Three threads, which never wait on each other in the normal course of a frame:
//  - The game thread polls the engine, owns the voice table, and every frame hands the
//    simulation thread its inputs and the audio thread a RenderFrame.
//  - The simulation thread traces occlusion and paths, looks up reverb and keeps dynamic
//    geometry in the scene, and hands the audio thread its SimResults after every run.
//  - The audio thread (miniaudio's callback) renders from its latest copy of each.
// A fourth, the loader thread, decodes the map's precached sounds in the background so their
// first play does not decode on the game thread.
// Every handoff is a short copy under its own mutex, which the audio thread only ever tries
// to take. The game thread waits only on rare structural changes: for the simulation thread
// to finish its run (PauseSimulation) or the audio thread its block (FenceAudio).
#pragma once

#include <atomic>
#include <thread>

#include "igamesystem.h"
#include "mathlib/vector.h"
#include "engine/SndInfo.h"
#include "soundflags.h"
#include "tier0/threadtools.h"
#include "utldict.h"
#include "utlstring.h"
#include "utlvector.h"

#include "neo_audio_dynamic.h"
#include "neo_audio_probes.h"
#include "neo_hrtf_local_sound.h"
#include "neo_spatializer.h"

class CNeoAudioGeometry;
class INetworkStringTable;
struct ma_device;

class CNeoHrtfSystem : public CAutoGameSystemPerFrame
{
public:
	static constexpr int kSampleRate = 48000;
	static constexpr int kFrameSize = 512;
	static constexpr int kMaxVoices = 128;
	static constexpr int kMaxErrorLen = 256;

	CNeoHrtfSystem() : CAutoGameSystemPerFrame("CNeoHrtfSystem") {}

	void Shutdown() override;
	void LevelInitPreEntity() override;
	void LevelInitPostEntity() override;
	void LevelShutdownPreEntity() override;
	void LevelShutdownPostEntity() override;
	void Update(float frametime) override;

	// Audio thread entry point (miniaudio data callback).
	void Render(float *pOutInterleaved, int frameCount);

	// Debug: writes the acoustic scene as <game dir>/hrtf_scene_<map>.obj.
	void SaveSceneObj();

	// (Re)bakes the current map's probes in the background, replacing any cached bake.
	void StartBake();
	void CancelBake();

	// The local player's sound `guid`, just played, is to be re-rendered from `place`
	// (CNeoHrtfLocalSoundScope).
	void MarkLocalSound(int guid, NeoHrtfLocalSound place);

private:
	// Decoded once per file, by the loader thread when precached or by the game thread on first
	// play, whichever claims it first. Immutable once ready, so the audio thread may read it without
	// locking for as long as a voice references it.
	struct CachedSound
	{
		enum State
		{
			Queued, // not decoded; whoever moves it to Loading decodes it
			Loading,
			Ready, // decoded, or failed with no samples
		};

		// Set by the game thread before anyone can decode it.
		CUtlString m_name;
		soundlevel_t m_scriptLevel = SNDLVL_NORM; // loudest scripted level of the file, the voice default
		std::atomic<int> m_state{ Queued };
		// Written while Loading. Stored at the file's own rate as 16-bit: a quarter of the memory of
		// 48 kHz float for the common 22 kHz files.
		CUtlVector<int16> m_samples; // mono at m_sampleRate; empty = unplayable, left to the engine
		int m_sampleRate = 0;
		int m_loopStart = -1; // sample index to loop back to, -1 for one-shots
		float m_distMult = 0.0f; // engine dist_mult of the sound's scripted level, 0 = no falloff
	};

	// How the audio thread is to play a voice this frame.
	struct VoiceParams
	{
		NeoSpatial::Vec3 m_origin; // metres
		float m_gain;
		float m_rate; // playback rate relative to kSampleRate, from the engine pitch
		float m_distanceGain; // the straight-line distance part of m_gain
		float m_falloffPerMetre; // the engine's distance law for this sound, for paths
	};

	// Game thread only.
	struct Voice
	{
		// Game thread only.
		bool m_bInUse = false;
		int m_guid = 0;
		// A new one for every sound the slot plays, so the other threads can tell a slot's new sound
		// from its last one. Never 0.
		uint32 m_generation = 0;
		const CachedSound *m_pSound = nullptr;
		NeoSpatial::VoiceHandle m_hSpatial = NeoSpatial::INVALID_VOICE;
		VoiceParams m_params = {};
		float m_sourceVolume = 0.0f; // engine channel volume before we muted it, restored on release
		float m_engineVolume = -1.0f; // what the engine copy was last set to, -1 = not yet
		float m_engineGain = 0.0f; // the engine's distance and pan gain for the channel, 0 = unknown
		bool m_bEngineVolumeJustSet = false; // set last poll, so the spatialized report may predate it
		bool m_bSeenThisPoll = false;
		float m_lastReportedVolume = 0.0f; // the engine's last reports for the channel, for the debug overlay
		float m_lastSpatializedVolume = 0.0f;
		// What the engine attenuates the channel by: per voice, since one file can be played at
		// different levels (e.g. ambient_generics with different radii).
		soundlevel_t m_soundLevel = SNDLVL_NORM;
		float m_distMult = 0.0f; // engine dist_mult for m_soundLevel, 0 = no falloff
		bool m_bLevelFromEmitter = false; // from a networked ambient_generic rather than the scripts
		NeoHrtfLocalSound m_localSound = NeoHrtfLocalSound::None; // the local player's own, and where from
		int m_soundSource = 0; // the entity playing it (SndInfo_t::m_nSoundSource)
		// Where its paths start, for as long as it stays at m_pathQueryOrigin: most sounds never move.
		bool m_bPathQueried = false;
		NeoSpatial::Vec3 m_pathQueryOrigin = {};
		bool m_bPathReachable = false;
		NeoSpatial::Vec3 m_pathOrigin = {};
	};

	struct PendingVoice
	{
		int m_soundIndex; // into m_activeSounds
		const CachedSound *m_pSound;
		int m_voiceIndex; // slot it was given, -1 if the backend refused a voice
		NeoHrtfLocalSound m_localSound;
	};

	struct LocalSoundMark
	{
		int m_guid;
		NeoHrtfLocalSound m_place;
		double m_time; // Plat_FloatTime when marked
	};

	// A backend voice the audio thread may still be rendering, released once it has moved on to
	// render frame m_sequence, the first without it.
	struct RetiredVoice
	{
		NeoSpatial::VoiceHandle m_hSpatial;
		uint32 m_sequence;
	};

	// Game thread → audio thread, every frame.
	struct RenderVoice
	{
		const CachedSound *m_pSound = nullptr; // null = slot idle
		NeoSpatial::VoiceHandle m_hSpatial = NeoSpatial::INVALID_VOICE;
		uint32 m_generation = 0;
		VoiceParams m_params = {};
	};
	struct RenderFrame
	{
		uint32 m_sequence = 0;
		NeoSpatial::Listener m_listener = {};
		float m_reverbGain = 0.0f; // wet level on top of each voice's own gain
		bool m_bReverbDecorrelate = true; // false: both ears get the same tail, inside the head
		RenderVoice m_voices[kMaxVoices];
	};

	// Game thread → simulation thread, every frame. Voices are listed by slot.
	struct SimVoiceInput
	{
		int m_slot;
		uint32 m_generation;
		NeoSpatial::Vec3 m_origin; // raised for occlusion (kHrtfOcclusionLiftUnits)
		uint32 m_ignoreGroup; // the dynamic geometry it is part of, not occluded by
		float m_falloffPerMetre;
		bool m_bPathReachable; // within reach of a probe, from m_pathOrigin
		NeoSpatial::Vec3 m_pathOrigin;
		float m_maxPathLevel; // what its paths are capped at: its level in plain view
	};
	struct SimFrame
	{
		uint32 m_sceneEpoch = 0; // m_sceneEpoch when built; a frame from an older scene is dropped
		NeoSpatial::Listener m_listener = {};
		bool m_bOcclusion = false;
		bool m_bPathing = false; // the listener is within reach of a probe and its batch is known
		int32 m_pathBatch = 0;
		NeoSpatial::Listener m_pathListener = {}; // pulled inside the nearest probe
		bool m_bReverb = false;
		int m_count = 0;
		SimVoiceInput m_voices[kMaxVoices];
	};
	struct SimInput
	{
		SimFrame m_frame;
		CUtlVector<CNeoAudioDynamicGeometry::Placement> m_placements;
	};

	// Simulation thread → audio thread (and the debug overlay), after every run. By slot.
	struct SimVoiceResult
	{
		uint32 m_generation = 0; // the sound these are for, 0 = none
		NeoSpatial::DirectPath m_path;
		NeoSpatial::PathParams m_paths; // around obstacles, through baked probes
	};
	struct SimTimings
	{
		float m_totalMs = 0.0f;
		float m_dynamicMs = 0.0f;
		float m_directMs = 0.0f;
		float m_pathingMs = 0.0f;
		float m_reverbMs = 0.0f;
		float m_runsPerSecond = 0.0f;
	};
	// Where the game thread's time goes, for the debug overlay: per-frame averages over the last
	// second, and counts per second.
	struct GameTimings
	{
		float m_pollMs = 0.0f; // all of PollEngineSounds
		float m_activeSoundsMs = 0.0f; // enginesound->GetActiveSounds
		float m_setVolumeMs = 0.0f; // enginesound->SetVolumeByGuid
		float m_newVoicesMs = 0.0f; // loading sounds and starting voices
		float m_gatherMs = 0.0f; // dynamic geometry
		float m_pathReachMs = 0.0f; // pulling path ends inside probes (ProbeCoverage::PullInside)
		float m_setVolumes = 0.0f; // calls per second
		float m_newVoices = 0.0f; // voices started per second
	};

	struct SimResults
	{
		uint32 m_sequence = 0;
		NeoSpatial::ReverbParams m_reverb; // the room around the listener, from baked probes
		SimTimings m_timings;
		int m_dynamicInstances = 0;
		SimVoiceResult m_voices[kMaxVoices];
	};

	void StartDevice();
	void StopDevice();
	void ReleaseAllVoices();
	void PollEngineSounds();
	void BuildSimFrame();
	void PublishRenderFrame();
	void ReleaseRetiredVoices();
	void BuildScene();
	void ReleaseScene();
	void SetupProbes(const CNeoAudioGeometry &geometry);
	bool LoadBakedProbeLump(const CNeoAudioGeometry &geometry);
	bool LoadProbeCache();
	void UpdateBake();
	void SaveProbeCache();
	void DrawProbes() const;

	const CachedSound *FindOrLoadSound(const SndInfo_t &info);
	// The cache entry for a sound, made (queued, not decoded) if there is none yet. Game thread.
	CachedSound *GetCachedSound(const char *pszNormalisedName);
	// Decodes `sound` unless someone else has, waiting for it if the loader thread has it now.
	void EnsureDecoded(CachedSound &sound);
	// Fills a Loading sound's samples and loop. Any thread.
	static void DecodeSound(CachedSound &sound);
	// Queues the sounds added to the engine's precache table since the last call. Game thread.
	void PrecacheNewSounds();
	soundlevel_t LookupScriptLevel(const char *pszNormalisedName);
	void ResolveSoundLevel(Voice &voice, const SndInfo_t &info, const CachedSound &sound) const;

	Vector SoundPosition(const SndInfo_t &info, NeoHrtfLocalSound localSound) const;
	VoiceParams ComputeParams(const SndInfo_t &info, NeoHrtfLocalSound localSound, float sourceVolume, float distMult,
							  float outputScale) const;
	NeoHrtfLocalSound TakeLocalSoundMark(int guid);
	int FindVoice(int guid) const;
	void MuteEngineCopy(Voice &voice, const SndInfo_t &info, float spatializedTarget);
	void PrintDebug();

	// Waits for the audio thread to finish its block, then stops it rendering anything and releases
	// every retired voice: for before the voices' sounds are freed or the backend goes.
	void FenceAudio();

	// Loader thread.
	void StartLoaderThread();
	void StopLoaderThread();
	void LoaderThreadMain();
	// Empties the queue and waits for the decode in progress: before cached sounds are freed.
	void DrainLoader();

	// Simulation thread.
	void StartSimulationThread();
	void StopSimulationThread();
	void SimulationThreadMain();
	void RunSimulation(const SimInput &input, SimResults &results);
	// Starts a new scene epoch: frames built before it are dropped, and the simulation thread's
	// sources are forgotten without releasing them, for when the backend dropped them with the scene.
	// The voices' path starts, found among this scene's probes, are forgotten too. Only while the
	// simulation is paused.
	void ResetSimulation();

	// Audio thread.
	void RenderBlock();
	bool ReadVoiceSamples(int slot, const RenderVoice &voice);

	// Game thread state.
	ma_device *m_pDevice = nullptr;
	NeoSpatial::ISpatializer *m_pSpatializer = nullptr;
	char m_szStartError[kMaxErrorLen] = ""; // why the last start failed; non-empty blocks retries until re-enabled
	bool m_bSceneStale = false; // the spatializer has no scene for the current map yet
	int m_sceneTriangles = 0; // 0 = no scene
	NeoSpatial::ProbeSet m_probes; // the current map's probes, also what the backend's batches hold
	uint32 m_probeCacheKey = 0; // geometry + probe layout + format; a cached bake must match it
	char m_szProbeCachePath[MAX_PATH] = "";
	NeoSpatial::BakeState m_lastBakeState = NeoSpatial::BakeState::Idle;
	CUtlVector<SndInfo_t> m_activeSounds;
	CUtlVector<int> m_ignoredGuids; // sounds deliberately left to the engine, rechecked by guid only
	CUtlVector<int> m_ignoredGuidsNext;
	CUtlVector<LocalSoundMark> m_localSoundMarks; // marked but not yet seen in the engine's channel list
	CUtlDict<CachedSound *, int> m_cache;
	std::atomic<int64> m_cachedBytes{ 0 }; // samples of every decoded sound, for the debug overlay
	INetworkStringTable *m_pSoundPrecacheTable = nullptr; // this map's, once found
	int m_precacheTableCount = 0; // its entries queued so far
	bool m_bPrecaching = false; // cl_neo_hrtf_precache when last checked
	CUtlDict<soundlevel_t, int> m_soundLevels; // normalised wave name → loudest scripted level
	Voice m_voices[kMaxVoices];
	uint32 m_nextGeneration = 1;
	PendingVoice m_pending[kMaxVoices];
	CUtlVector<RetiredVoice> m_retiredVoices;
	NeoSpatial::BspTree m_bspTree; // the current map's, to find the listener's area (its probe batch)
	NeoSpatial::ProbeCoverage m_probeCoverage; // where Steam Audio's paths are actually recomputed
	Vector m_listenerOrigin;
	Vector m_listenerForward;
	Vector m_listenerRight;
	Vector m_listenerUp;
	NeoSpatial::Listener m_listener = {};
	RenderFrame m_renderNext; // built here, then copied to m_renderShared
	SimInput m_simNext; // built here, then handed to m_simShared
	SimResults m_debugResults; // the latest SimResults, copied for the debug overlay
	GameTimings m_gameTimings; // the last full second
	GameTimings m_gameTimingSums; // this second so far: seconds and counts
	int m_gameTimingFrames = 0;
	double m_gameTimingStart = 0.0;

	// Shared, each under its own mutex.
	CThreadMutex m_renderMutex;
	RenderFrame m_renderShared;
	CThreadMutex m_simInputMutex;
	SimInput m_simShared;
	bool m_bSimInputFresh = false; // m_simShared holds a frame the simulation has not run yet
	CThreadMutex m_simResultsMutex;
	SimResults m_simResultsShared;
	// Held by the simulation thread for each run, and by the game thread to keep it from starting
	// one while the scene, probes or bake change (PauseSimulation).
	CThreadMutex m_simRunMutex;
	uint32 m_sceneEpoch = 0; // written by the game thread only while holding m_simRunMutex
	// Held by the audio thread for each block, and by the game thread in FenceAudio.
	CThreadMutex m_renderBlockMutex;
	std::atomic<uint32> m_renderAcquired{ 0 }; // the sequence of the render frame the audio thread has
	CThreadEvent m_simWake; // a new SimInput, or time to quit
	std::atomic<bool> m_bSimQuit{ false };
	std::thread m_simThread;

	// Simulation thread only.
	SimInput m_simRun;
	SimResults m_simResults;
	CNeoAudioDynamicGeometry m_dynamicGeometry; // Gather runs on the game thread, the rest here
	NeoSpatial::SourceHandle m_simSources[kMaxVoices]; // by slot
	uint32 m_simSourceGenerations[kMaxVoices]; // the sound each slot's source was made for, 0 = none
	NeoSpatial::SourceHandle m_simHandles[kMaxVoices];
	NeoSpatial::Vec3 m_simOrigins[kMaxVoices];
	uint32 m_simIgnoreGroups[kMaxVoices];
	NeoSpatial::DirectPath m_simPaths[kMaxVoices];
	NeoSpatial::PathingVoice m_simPathingVoices[kMaxVoices];
	NeoSpatial::PathParams m_simPathing[kMaxVoices];
	double m_simRateWindowStart = 0.0;
	int m_simRateRuns = 0;
	float m_simRunsPerSecond = 0.0f;

	// Loader thread, and the game thread through the queue.
	CThreadMutex m_loadQueueMutex;
	CUtlVector<CachedSound *> m_loadQueue; // first in, first out from m_loadQueueHead
	int m_loadQueueHead = 0;
	// Held by the loader thread from taking a sound off the queue until it is decoded, so DrainLoader
	// can wait for it.
	CThreadMutex m_loadRunMutex;
	CThreadEvent m_loadWake;
	std::atomic<bool> m_bLoadQuit{ false };
	std::thread m_loadThread;

	// Audio thread only.
	RenderFrame m_render; // the latest render frame taken
	SimResults m_renderSim; // the latest results taken
	double m_cursors[kMaxVoices] = {}; // by slot
	bool m_finished[kMaxVoices] = {};
	uint32 m_renderGenerations[kMaxVoices] = {}; // the sound the cursor belongs to
	float m_carry[kFrameSize * 2]; // interleaved stereo block not yet handed to miniaudio
	int m_carryRead = 0;
	int m_carryAvailable = 0;
	float m_scratchMono[kFrameSize];
	float m_scratchLeft[kFrameSize];
	float m_scratchRight[kFrameSize];
};
