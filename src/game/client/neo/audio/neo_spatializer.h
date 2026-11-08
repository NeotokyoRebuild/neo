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

class ISpatializer
{
public:
	virtual ~ISpatializer() {}

	virtual void SetListener(const Listener &listener) = 0;

	// A voice holds per-source filter state (e.g. HRTF interpolation history).
	virtual VoiceHandle CreateVoice() = 0;
	virtual void ReleaseVoice(VoiceHandle voice) = 0;

	// Spatialise one block. `origin` is the source position in the listener's frame of reference.
	virtual void Process(VoiceHandle voice, const Vec3 &origin, const float *monoIn,
						 float *outLeft, float *outRight) = 0;
};

// Loads phonon.dll / libphonon.so from `phononLibraryPath` at runtime (it is never linked) and
// initialises it. Returns nullptr and fills errorOut on failure, so a missing or incompatible
// library only disables HRTF. Destroy the result with delete.
ISpatializer *CreateSteamAudioSpatializer(const char *phononLibraryPath, int sampleRate, int frameSize,
										  char *errorOut, int errorLen);

} // namespace NeoSpatial
