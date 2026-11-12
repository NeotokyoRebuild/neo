#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// func_audio_blocker: brushes that only block sound. The client reads them straight from the BSP for
// its acoustic scene (client/neo/audio/neo_audio_geometry.cpp), so there is nothing for the server to
// do: the model is never set, so nothing collides with the brushes, and the entity removes itself.
// Without a class here the engine would warn about an unknown entity on every map load.
class CNEOAudioBlocker : public CLogicalEntity
{
	DECLARE_CLASS(CNEOAudioBlocker, CLogicalEntity);

public:
	virtual void Spawn() override
	{
		BaseClass::Spawn();
		UTIL_Remove(this);
	}
};

LINK_ENTITY_TO_CLASS(func_audio_blocker, CNEOAudioBlocker);
