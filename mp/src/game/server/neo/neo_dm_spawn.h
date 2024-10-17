#pragma once

#include "mathlib/vector.h"
#include "convar.h"

namespace DMSpawn
{

struct Info
{
	Vector pos;
	float lookY;
};

bool HasDMSpawn();
Info GiveNextSpawn();

}

void DMSpawnComCallbackLoad([[maybe_unused]] const CCommand &command = CCommand{});
