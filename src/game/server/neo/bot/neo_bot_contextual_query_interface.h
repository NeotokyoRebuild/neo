#pragma once

class CNEOBot;

class CNEOBotContextualQueryInterface
{
public:
	~CNEOBotContextualQueryInterface() {}

	// Should the bot walk?
	virtual QueryResultType ShouldWalk(const CNEOBot *me) const = 0;
};

