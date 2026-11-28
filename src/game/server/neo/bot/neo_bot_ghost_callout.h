#pragma once

// A ghost carrier's teammates learn the position of the enemy the carrier is looking at,
// as a human carrier's voice callout would tell them
namespace NEOBotGhostCallout
{
	// Checks the carrier's aim once per callout interval
	void Update();

	// How long a carrier must hold its aim on an enemy for one callout to read it
	float GetMinAimHoldTime();

	// Forget the callout time and booted carry, e.g. on a map change where curtime restarts
	void Reset();
}
