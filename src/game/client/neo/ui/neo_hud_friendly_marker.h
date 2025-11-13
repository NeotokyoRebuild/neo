#pragma once

#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "neo_gamerules.h"
#include "neo_hud_worldpos_marker.h"

enum NeoIFFMarkerSegment
{
	NEOIFFMARKER_SEGMENT_I_INITIALOFFSET = 0,
	NEOIFFMARKER_SEGMENT_B_SHOWDISTANCE,
	NEOIFFMARKER_SEGMENT_B_VERBOSEDISTANCE,
	NEOIFFMARKER_SEGMENT_B_SHOWSQUADMARKER,
	NEOIFFMARKER_SEGMENT_FL_SQUADMARKERSCALE,
	NEOIFFMARKER_SEGMENT_B_SHOWHEALTHBAR,
	NEOIFFMARKER_SEGMENT_B_SHOWHEALTH,
	NEOIFFMARKER_SEGMENT_B_SHOWNAME,
	NEOIFFMARKER_SEGMENT_B_PREPENDCLANTAGTONAME,
	NEOIFFMARKER_SEGMENT_I_MAXNAMELENGTH,

	NEOIFFMARKER_SEGMENT__TOTAL,
};

#define NEO_FRIENDLY_MARKER_DEFAULT "0;1;1;1;0.5;1;1;1;1;32;"
constexpr int NEO_IFFMARKER_SEQMAX = 32;

struct FriendlyMarkerInfo
{
	int iInitialOffset;
	bool bShowDistance;
	bool bVerboseDistance;
	bool bShowSquadMarker;
	float flSquadMarkerScale;
	bool bShowHealthBar;
	bool bShowHealth;
	bool bShowName;
	bool bPrependClantagToName;
	int iMaxNameLength;
};

enum NeoIFFMarkerOption
{
	NEOIFFMARKER_OPTION_FRIENDLY = 0,
	NEOIFFMARKER_OPTION_FRIENDLY_XRAY,
	NEOIFFMARKER_OPTION_SQUAD,
	NEOIFFMARKER_OPTION_SQUAD_XRAY,
	NEOIFFMARKER_OPTION_PLAYER,
	NEOIFFMARKER_OPTION_PLAYER_XRAY,

	NEOIFFMARKER_OPTION_TOTAL,
};

class CNEOHud_FriendlyMarker : public CNEOHud_WorldPosMarker
{
	DECLARE_CLASS_SIMPLE(CNEOHud_FriendlyMarker, CNEOHud_WorldPosMarker)

public:
	CNEOHud_FriendlyMarker(const char *pElemName, vgui::Panel *parent = NULL);
	
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;

	bool ParseFriendlyMarker(NeoIFFMarkerOption option, const char *pszSequence);

protected:
	virtual void DrawNeoHudElement() override;
	void DrawPlayerForTeam(C_Team *team, const C_NEO_Player *localPlayer, const C_NEO_Player *pTargetPlayer) const;
	virtual ConVar* GetUpdateFrequencyConVar() const override;
	virtual void UpdateStateForNeoHudElementDraw() override;

private:
	int m_iIconWidth, m_iIconHeight;
	
	FriendlyMarkerInfo m_szMarkerSettings[NEOIFFMARKER_OPTION_TOTAL] = {};

	int m_x0[MAX_PLAYERS];
	int m_x1[MAX_PLAYERS];
	int m_y0[MAX_PLAYERS];
	int m_y1[MAX_PLAYERS];

	vgui::HTexture m_hStarTex = 0UL;
	vgui::HTexture m_hNonStarTex = 0UL;
	vgui::HTexture m_hUniqueTex = 0UL;
	vgui::HFont m_hFont = 0UL;

	void DrawPlayer(Color teamColor, C_NEO_Player *player, const C_NEO_Player *localPlayer) const;
	static Color GetTeamColour(int team);
};
