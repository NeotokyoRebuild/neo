#pragma once

class CNEOBot;

class CNEOBotContextualQueryInterface
{
public:
	~CNEOBotContextualQueryInterface() {}

	// Should the bot walk?
	virtual QueryResultType ShouldWalk(const CNEOBot *me, const QueryResultType qShouldAimQuery) const = 0;

	// Should the bot use ADS/Aim down sight?
	virtual QueryResultType ShouldAim(const CNEOBot *me, const bool bWepHasClip) const = 0;
};

