#pragma once

// A ghost carrier's teammate bots learn where the enemy the carrier is looking at is,
// as a human carrier's voice callout would tell them
namespace NEOBotGhostCallout
{
	// Checks the carrier's aim once per callout interval
	void Update();

	// Forget the callout time and booted carry, e.g. on a map change where curtime restarts
	void Reset();
}
