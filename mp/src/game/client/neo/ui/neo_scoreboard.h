#ifndef NEO_SCOREBOARD_H
#define NEO_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include <clientscoreboarddialog.h>

// NEO TODO (Rain): this is temporary to get some utility drawing going.
// We should ultimately inherit and implement directly from CClientScoreBoardDialog.
#include "hl2mpclientscoreboard.h"

class CNEOScoreBoard : public CHL2MPClientScoreBoardDialog
{
	DECLARE_CLASS_SIMPLE(CNEOScoreBoard, CHL2MPClientScoreBoardDialog);

public:
	CNEOScoreBoard(IViewPort *pViewPort);
	~CNEOScoreBoard();

protected:
	// scoreboard overrides
	virtual void InitScoreboardSections() OVERRIDE;
	virtual void UpdatePlayerInfo() OVERRIDE;
	virtual void UpdateTeamInfo() OVERRIDE;
	virtual bool GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo) OVERRIDE;
	virtual void SetVisible(bool state) OVERRIDE;

	// vgui overrides
	virtual void PaintBackground() OVERRIDE;
	virtual void PaintBorder() OVERRIDE;
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) OVERRIDE;

	bool IsTeamplay(void) { return true; }

private:
	virtual void AddHeader() OVERRIDE; // add the start header of the scoreboard
	virtual void AddSection(int teamType, int teamNumber) OVERRIDE; // add a new section header for a team

	int GetSectionFromTeamNumber(int teamNumber);

	// These mimic hl2mpclientscoreboard values, for now
	enum {
		NEO_NAME_WIDTH = 264,
		NEO_CLASS_WIDTH = 56,
		NEO_SCORE_WIDTH = 40,
		NEO_DEATH_WIDTH = 46,
		NEO_PING_WIDTH = 46,
		NEO_VOICE_WIDTH =  40,
		NEO_FRIENDS_WIDTH = 24,
	};

	Color m_bgColor;
	Color m_borderColor;
};

#endif // NEO_SCOREBOARD_H