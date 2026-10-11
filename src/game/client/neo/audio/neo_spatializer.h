// NEO HRTF: backend-neutral spatializer contract, the whole boundary between NT;RE and a
// spatial-audio library. It has no Source SDK dependencies, so a backend builds and tests outside
// the game and a library upgrade never touches game code.
//
// Conventions
//  - Positions are in Source's world frame (+Z up) scaled to METRES; the backend converts axes.
//  - Audio is mono-in, stereo-out, 32-bit float, non-interleaved, in blocks of the frame size
//    the backend was created with.
//  - Creation, destruction, CreateVoice and ReleaseVoice run on the game thread; SetListener and
//    Process on the audio thread. The caller guarantees they never overlap.
//  - Process() OVERWRITES the outputs at unity gain: distance attenuation and volume are the
//    caller's, so the distance model can change without touching a backend.
//  - Scene, probes and simulation (SetSceneGeometry, the dynamic geometry calls, the probe calls,
//    SimulateDirect, SaveSceneObj) are game-thread state that Process never reads, so those calls need not be serialised with the
//    audio thread. Their results reach Process only as the DirectPath the caller passes it.
//  - A bake runs on the backend's own thread; every other call that touches the scene or probe
//    batches cancels and joins it first, so the caller never has to.
#pragma once

#include <cstdint>

namespace NeoSpatial
{

struct Vec3
{
	float x, y, z;
};

struct Listener
{
	Vec3 origin;
	Vec3 forward;
	Vec3 right;
	Vec3 up;
};

typedef uint32_t VoiceHandle;
static constexpr VoiceHandle INVALID_VOICE = 0;

static constexpr int NUM_ACOUSTIC_BANDS = 3; // centred on 400 Hz, 2.5 kHz and 15 kHz

// Fractions of sound energy, each between 0 and 1.
struct AcousticMaterial
{
	float absorption[NUM_ACOUSTIC_BANDS]; // absorbed on reflection
	float scattering; // reflected diffusely rather than specularly
	float transmission[NUM_ACOUSTIC_BANDS]; // passed through the surface
};

// Static world geometry as an indexed triangle list, in the same frame and units as positions.
// Front faces wind counter-clockwise seen from the front, i.e. the normal is (b - a) x (c - a).
// The backend copies what it needs; the arrays only have to outlive the call.
struct SceneGeometry
{
	const Vec3 *vertices = nullptr;
	int numVertices = 0;
	const int32_t *triangles = nullptr; // 3 vertex indices per triangle
	const int32_t *materialIndices = nullptr; // one per triangle, into materials
	int numTriangles = 0;
	const AcousticMaterial *materials = nullptr;
	int numMaterials = 0;
};

// Places a dynamic model in the world, in the same frame and units as positions:
// world = rotation * local + translation, with rotation a 3x3 rotation matrix (row-major).
struct Transform
{
	float rotation[3][3];
	Vec3 translation;
};

// Dynamic geometry (see ISpatializer::CreateDynamicModel).
typedef uint32_t DynamicModel;
typedef uint32_t DynamicInstance;
static constexpr DynamicModel INVALID_DYNAMIC_MODEL = 0;
static constexpr DynamicInstance INVALID_DYNAMIC_INSTANCE = 0;
// Instances belong to a caller-chosen group (e.g. a train and everything riding on it), which a
// voice can ignore for occlusion; this group is none.
static constexpr uint32_t NO_DYNAMIC_GROUP = 0;

// Probes laid out as consecutive batches: batch b holds probes [batchStarts[b], batchStarts[b + 1]).
// Each batch carries a caller-chosen id (e.g. the BSP area it covers) that survives serialisation.
struct ProbeLayout
{
	const Vec3 *centres = nullptr;
	int numProbes = 0;
	float radius = 0.0f; // each probe's sphere of influence
	const int32_t *batchStarts = nullptr; // numBatches + 1 entries
	const int32_t *batchIds = nullptr;
	int numBatches = 0;
};

enum class BakeState
{
	Idle, // nothing baked or baking
	Running,
	Done, // every batch holds baked reverb and pathing
	Failed, // cancelled or a batch could not be baked; batches keep whatever finished
};

// The reverb of the space around the listener, from baked probes. Invalid until the listener has
// been near a baked probe on this map; the backend holds the last valid value while the listener
// is away from probes (e.g. mid-jump) rather than dropping the room.
struct ReverbParams
{
	bool valid = false;
	float reverbTimes[NUM_ACOUSTIC_BANDS] = {}; // RT60 in seconds
};

// What the scene does to the straight line from a voice to the listener. The default is an
// unobstructed path, which is also what every voice gets while there is no scene.
struct DirectPath
{
	float occlusion = 1.0f; // fraction of the source the listener can see, 1 = unoccluded
	float transmission[NUM_ACOUSTIC_BANDS] = { 1.0f, 1.0f, 1.0f }; // fraction passing through the occluders
};

static constexpr int PATHING_AMBISONIC_COEFFS = 4; // first order: enough to place a path's direction

// The sound reaching the listener around obstacles, along paths through baked probes, as a
// first-order sound field in world space (the backend rotates it to the listener). Invalid when
// pathing has nothing for the voice: no baked probes yet, or the listener's batch is unknown.
struct PathParams
{
	bool valid = false;
	float eq[NUM_ACOUSTIC_BANDS] = { 1.0f, 1.0f, 1.0f }; // spectral loss from bending around corners
	float sh[PATHING_AMBISONIC_COEFFS] = {}; // includes distance attenuation along the paths
};

// A voice to path, with its distance law: gain = min(1, 1 / (distance * falloffPerMetre)), or 0
// below the minimum gain passed alongside; falloffPerMetre 0 means no falloff. The paths are longer
// than the straight line, so this is what makes a sound heard around a corner quieter.
struct PathingVoice
{
	VoiceHandle voice;
	Vec3 origin;
	float falloffPerMetre;
};

// Everything Process needs for one voice and block.
struct VoiceRender
{
	DirectPath direct;
	PathParams paths;
	// The share of the voice the direct path does not carry, 1 - direct.occlusion, is what is sent
	// along the paths, so a visible source is never doubled. pathGain scales that pathed sound: the
	// caller multiplies the whole output by its own (straight-line) distance gain, which the paths,
	// carrying their own attenuation, must not get twice.
	float pathGain = 0.0f;
	// The direct-filtered block times this goes to the reverb, and so does the pathed sound at the
	// level the paths deliver it, so a source out of sight still excites the listener's room.
	float reverbSend = 0.0f;
};

class ISpatializer
{
public:
	virtual ~ISpatializer() {}

	virtual void SetListener(const Listener &listener) = 0;

	// A voice holds per-source filter state (e.g. HRTF interpolation history).
	virtual VoiceHandle CreateVoice() = 0;
	virtual void ReleaseVoice(VoiceHandle voice) = 0;

	// Spatialise one block: the direct path, filtered by render.direct, from `origin` (world
	// position); plus the occluded share along render.paths, from the directions the paths arrive
	// from. The direct-filtered block, times render.reverbSend, is added to this block's reverb input.
	virtual void Process(VoiceHandle voice, const Vec3 &origin, const VoiceRender &render, const float *monoIn,
						 float *outLeft, float *outRight) = 0;

	// Once per block, after Process for every voice: renders the reverb of everything sent this
	// block (and the tail of earlier ones) as a stereo pair, overwriting the outputs, and clears
	// the reverb input. Silent while params are invalid. bDecorrelate gives each ear its own take
	// on the tail so it surrounds the listener; without it both ears hear the same tail, inside
	// the head.
	virtual void ProcessReverb(const ReverbParams &params, bool bDecorrelate, float *outLeft, float *outRight) = 0;

	// Replaces the static scene; geometry without triangles just removes the current one.
	// On failure there is no scene, and errorOut says why. Drops every dynamic model and instance.
	virtual bool SetSceneGeometry(const SceneGeometry &geometry, char *errorOut, int errorLen) = 0;

	// Dynamic geometry: things that move or come and go (doors, trains). A model is a mesh in its
	// own frame (geometry as for SetSceneGeometry), placed in the scene by any number of instances.
	// Dynamic geometry occludes from the next simulation on (pathing ignores it), but is never baked:
	// probes and their bake only see the static scene, so baked data stays valid however it moves.
	// Models need a scene; INVALID_DYNAMIC_MODEL if there is none or the model could not be made.
	virtual DynamicModel CreateDynamicModel(const SceneGeometry &geometry) = 0;
	// Releases the model along with any instances of it still in the scene.
	virtual void ReleaseDynamicModel(DynamicModel model) = 0;
	virtual DynamicInstance CreateDynamicInstance(DynamicModel model, const Transform &transform, uint32_t group) = 0;
	virtual void MoveDynamicInstance(DynamicInstance instance, const Transform &transform) = 0;
	virtual void ReleaseDynamicInstance(DynamicInstance instance) = 0;
	virtual int NumDynamicInstances() const = 0;

	// Traces each voice's origin (world positions, as for Process) to the listener through the scene
	// and writes one path per voice. Invalid handles, and every voice while there is no scene, get
	// the default unobstructed path. ignoreGroups (optional, one per voice) names a group of dynamic
	// instances each voice is traced as if they were not there, e.g. the door it is the sound of; each
	// group ignored costs a simulation run of its own.
	virtual void SimulateDirect(const Listener &listener, const VoiceHandle *voices, const Vec3 *origins,
								const uint32_t *ignoreGroups, DirectPath *pathsOut, int count) = 0;

	// Looks up the baked reverb at the listener. Probe batches only take part once their bake is
	// complete (finished or loaded); until then the result stays invalid.
	virtual void SimulateReverb(const Listener &listener, ReverbParams *out) = 0;

	// Finds the paths from each voice to the listener through the baked batch with id batchId (the
	// one covering the listener). Writes one result per voice; all invalid until that batch's bake is
	// complete. A voice the listener can see gets its direct line as its path. Paths are used as
	// baked: dynamic geometry does not block them.
	virtual void SimulatePathing(const Listener &listener, int32_t batchId, float minGain, const PathingVoice *voices,
								 PathParams *out, int count) = 0;

	// Debug: writes the scene as <fileBaseName>.obj and .mtl. False if there is no scene.
	virtual bool SaveSceneObj(const char *fileBaseName) = 0;

	// Probes at `height` above the floors inside an axis-aligned box, about `spacing` apart, from
	// the backend's own generator. Writes up to maxOut centres and returns how many there were
	// (which may exceed maxOut); 0 without a scene.
	virtual int GenerateFloorProbes(const Vec3 &boxMins, const Vec3 &boxMaxs, float spacing, float height,
									Vec3 *centresOut, int maxOut) = 0;

	// Replaces the probe batches (dropping any baked data). An empty layout just removes them.
	virtual bool SetProbeBatches(const ProbeLayout &layout, char *errorOut, int errorLen) = 0;

	// Starts baking every batch on a background thread, using up to numThreads threads: first
	// listener-centric reverb, then the paths between its probes. False if there are no batches.
	virtual bool StartBake(int numThreads) = 0;
	virtual void CancelBake() = 0;
	// progressOut, if not null, gets the overall fraction done.
	virtual BakeState GetBakeState(float *progressOut) = 0;

	// Serialises every batch, probes and baked data, into a backend-owned buffer valid until the
	// next call that changes the batches. Null (and size 0) while a bake runs or with no batches.
	virtual const uint8_t *SerializeProbeBatches(int64_t *sizeOut) = 0;
	// Replaces the batches with ones from SerializeProbeBatches; on failure there are none.
	virtual bool LoadProbeBatches(const uint8_t *data, int64_t size, char *errorOut, int errorLen) = 0;

	virtual int NumProbeBatches() const = 0;
	virtual int NumProbes() const = 0;

	// The backend library's version, recorded with baked data for diagnostics.
	virtual uint32_t GetLibraryVersion() const = 0;
};

// Loads phonon.dll / libphonon.so from `phononLibraryPath` at runtime (it is never linked) and
// initialises it. Returns nullptr and fills errorOut on failure, so a missing or incompatible
// library only disables HRTF. Destroy the result with delete.
ISpatializer *CreateSteamAudioSpatializer(const char *phononLibraryPath, int sampleRate, int frameSize,
										  char *errorOut, int errorLen);

} // namespace NeoSpatial
