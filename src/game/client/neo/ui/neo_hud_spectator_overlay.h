#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImageList.h>
#include "neo_avatar.h"

extern ConVar cl_neo_hud_spectator_overlay_enabled;
extern ConVar cl_neo_hud_spectator_overlay_jinrai_name;
extern ConVar cl_neo_hud_spectator_overlay_nsf_name;
extern ConVar cl_neo_hud_spectator_overlay_jinrai_logo;
extern ConVar cl_neo_hud_spectator_overlay_nsf_logo;
extern ConVar cl_neo_hud_spectator_overlay_jinrai_matches_won;
extern ConVar cl_neo_hud_spectator_overlay_nsf_matches_won;

static constexpr const int COMP_MATCHES_WON_MAX = 2;

struct SpectatorPlayerCard
{
	int iUserID = 0;
	int iEntIndex = 0;
	int iTeam = 0;
	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH] = {};
	// As the card shows them: the name cut to fit beside the class name, and the class name in capitals
	wchar_t wszDisplayName[MAX_PLAYER_NAME_LENGTH] = {};
	wchar_t wszClassName[32] = {};
	// NEO NOTE (nullsystem): wcaWeaponIcons is NOT used as a string
	// but individual characters!
	wchar_t wcaWeaponIcons[MAX_WEAPONS] = {};
	int iTotalWeaponIcons = 0;
	int iWeaponIdxActive = 0;
	int iWeaponIdxPrimary = 0;
	// The gap between this row's weapon icons, tighter when they would come too close to the stats
	int iWeaponGap = 0;
	int iXP = 0;
	int iDeath = 0;
	int iClass = 0;
	int iHP = 0;
	int iRoundKills = 0;
	bool bAlive = false;
	// Used for animations/fading
	float flLastAttackTime = 0.0f;
	float flLastAliveTime = 0.0f;
	float flLastHPChangeTime = 0.0f;
	int iLastHP = 0;
};

// Client-side spectator-only HUD overlay showing both teams players infos
// Mainly for competitve streams
class CNEOHud_SpectatorOverlay : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_SpectatorOverlay, EditablePanel);

public:
	CNEOHud_SpectatorOverlay(const char *pElementName, vgui::Panel *parent = nullptr);
	CNEOHud_SpectatorOverlay(const CNEOHud_SpectatorOverlay& other) = delete;
	virtual ~CNEOHud_SpectatorOverlay();

	void ClearAll();
	void Init() final;
	void LevelShutdown() final;
	void FireGameEvent(IGameEvent *event) final;
	void ApplySchemeSettings(vgui::IScheme *pScheme) final;
	void Paint() final;

	SpectatorPlayerCard m_cards[MAX_PLAYERS_ARRAY_SAFE] = {};
	// m_avatars separate as m_cards always cleared
	NeoAvatar m_avatars[MAX_PLAYERS_ARRAY_SAFE] = {};
	int m_iCardsSize = 0;

	enum ESpecType
	{
		SPECTYPE_DIRECT = 0,
		SPECTYPE_SELECT,
	};
	enum EDirection
	{
		DIRECTION_NIL = 0,
		DIRECTION_NEXT,
		DIRECTION_PREV,
	};
	int SpecTargetNextEntIdx(const ESpecType eSpecType, EDirection eDirection) const;
	int m_iEntIndexSelect = 0;

	int m_iTexLogoJinrai = -1;
	int m_iTexLogoNSF = -1;

protected:
	void UpdateStateForNeoHudElementDraw() final;
	void DrawNeoHudElement() final;
	ConVar *GetUpdateFrequencyConVar() const final;

private:
	// A team's stats columns, worked out in the update step, since the cards change only there
	struct TeamStatsLayout
	{
		int iXPWide = 0;
		int iDeathWide = 0;
		int iGroupGap = 0;
	};

	void SetCardNames(SpectatorPlayerCard &card, const int iNameRowWide) const;
	void UpdateTeamStatsLayout(const int iRowRoom);
	// Text held to the card's left or right edge, so a card and its mirror image share one call
	void DrawCardText(const vgui::HFont font, const bool bAlignLeft, const int xLeft, const int xRight, const int yBaseline,
			const wchar_t *pwszText) const;
	int GetCardWeaponsWide(const SpectatorPlayerCard &card, const int iWeaponGap) const;
	void DrawCardNameRow(const SpectatorPlayerCard &card, const bool bIsLeftSide,
			const int x, const int y, const int wide, const int iAvatarSize, const int iEdgePad, const Color &colorText) const;
	void DrawCardStatsRow(const SpectatorPlayerCard &card, const bool bIsLeftSide,
			const int x, const int y, const int wide, const int iAvatarSize, const int iEdgePad, const Color &colorText) const;
	void DrawPlayerCard(const int iPlayerIdx,
			const bool bIsLeftSide,
			const int x, const int y,
			const int wide, const int tall,
			const Color accentColor, const float flWideAs43) const;

	CPanelAnimationVar(vgui::HFont, m_hNameFont, "NameFont", "NHudSpectatorOverlayName");
	CPanelAnimationVar(vgui::HFont, m_hInfoFont, "InfoFont", "NHudSpectatorOverlayInfo");
	CPanelAnimationVar(vgui::HFont, m_hClassFont, "ClassFont", "NHudSpectatorOverlayClass");
	CPanelAnimationVar(vgui::HFont, m_hRKHPFont, "RKHPFont", "NHudSpectatorOverlayRoundKillHPFront");
	CPanelAnimationVar(vgui::HFont, m_hRKHPBackFont, "RKHPBackFont", "NHudSpectatorOverlayRoundKillHPBack");
	CPanelAnimationVar(vgui::HFont, m_hGhostFont, "GhostFont", "NHudSpectatorOverlayGhost");
	CPanelAnimationVar(vgui::HFont, m_hSmallWeaponsFont, "SmallWeaponsFont", "NHudSpectatorOverlaySmallWeapons");
	CPanelAnimationVar(vgui::HFont, m_hTeamNameFont, "TeamNameFont", "HudHintTextLarge");
	CPanelAnimationVarAliasType(int, m_iDeadTexture, "DeadTexture", "vgui/hud/kill_kill", "textureid");

	int m_iTeamPlayersCount[TEAM__TOTAL] = {};
	TeamStatsLayout m_teamStats[TEAM__TOTAL] = {};
	int m_iRankIconWide = 0;
	int m_iDeathIconWide = 0;
};

extern CNEOHud_SpectatorOverlay *g_pNeoHudSpecOverlay;

