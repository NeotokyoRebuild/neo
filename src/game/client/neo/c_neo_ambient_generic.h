#pragma once

#include "c_baseentity.h"
#include "utlvector.h"

// Client half of ambient_generic (server/sound.cpp). The engine plays the sound itself; this only
// carries what an engine channel does not: the sound level the server emits it at, so the HRTF
// renderer can attenuate it as the engine does. Every one is transmitted, wherever it is.
// Being a client entity also makes it what the engine asks for the position of sounds emitted from
// its index, so it receives the base entity origin like any other entity.
class C_AmbientGeneric : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_AmbientGeneric, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_AmbientGeneric();
	~C_AmbientGeneric() override;

	bool ShouldDraw() override { return false; }

	// The entity index the sound is emitted from (the channel's sound source), -1 before the server
	// has resolved it.
	int GetEmitterIndex() const { return m_nNetSoundSourceIndex; }
	// soundlevel_t the channel is attenuated by, or -1 when it comes from a sound script instead.
	int GetNetSoundLevel() const { return m_nNetSoundLevel; }
	// As the map gives it: may carry sound characters and lack the sound/ prefix.
	const char *GetSoundFile() const { return m_szNetSoundFile; }

	static int Count() { return s_ambients.Count(); }
	static const C_AmbientGeneric *Get(int index) { return s_ambients[index]; }

private:
	int m_nNetSoundSourceIndex = -1;
	int m_nNetSoundLevel = -1;
	char m_szNetSoundFile[MAX_PATH] = "";

	static CUtlVector<C_AmbientGeneric *> s_ambients;
};
