#pragma once

class CNEOBotContextualQueryInterface
{
public:
	~CNEOBotContextualQueryInterface() {}

	// Should the bot walk?
	virtual QueryResultType ShouldWalk(const INextBot *me) const = 0;

	// Should the bot use ADS/Aim down sight?
	virtual QueryResultType ShouldAim(const INextBot *me) const = 0;
};

