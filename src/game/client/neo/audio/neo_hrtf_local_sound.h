// NEO HRTF: which of the local player's own sounds are re-rendered with HRTF. The engine's channel
// list (SndInfo_t) cannot carry a flag of our own, so the client marks a sound by its guid as it
// plays it, and the HRTF system picks the mark up when it polls the channels. Unmarked sounds from
// the local player (the vision toggle, the use key, weapon pickups, ...) stay with the engine,
// non-positional as before.
#pragma once

// Where a marked sound is heard from.
enum class NeoHrtfLocalSound
{
	None,
	Feet, // where the engine plays it; footsteps are emitted at the feet
	Weapon, // the viewmodel weapon, just ahead of and below the eyes
	Body, // the chest, below the eyes
};

// Marks the sound played inside the scope (the last one, if several are) as the local player's own,
// to be re-rendered with HRTF from `place`. Does nothing while HRTF is off, or when nothing played
// (e.g. a predicted sound suppressed on a repeat of the same command).
class CNeoHrtfLocalSoundScope
{
public:
	explicit CNeoHrtfLocalSoundScope(NeoHrtfLocalSound place);
	~CNeoHrtfLocalSoundScope();

	CNeoHrtfLocalSoundScope(const CNeoHrtfLocalSoundScope &) = delete;
	CNeoHrtfLocalSoundScope &operator=(const CNeoHrtfLocalSoundScope &) = delete;

private:
	NeoHrtfLocalSound m_place;
	int m_guidBefore;
};
