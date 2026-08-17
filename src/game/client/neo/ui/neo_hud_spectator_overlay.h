#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

class CAvatarImage;

struct SpectatorPlayerCard
{
	int iUserID = 0;
	int iEntIndex = 0;
	int iTeam = 0;
	wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH] = {};
	// NEO NOTE (nullsystem): wcaWeaponIcons is NOT used as a string
	// but individual characters!
	wchar_t wcaWeaponIcons[MAX_WEAPONS] = {};
	int iTotalWeaponIcons = 0;
	int iWeaponIdxActive = 0;
	int iWeaponIdxPrimary = 0;
	int iXP = 0;
	int iDeath = 0;
	int iClass = 0;
	int iHP = 0;
	int iRoundKills = 0;
	bool bAlive = false;
	CAvatarImage *pAvatar = nullptr;
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

protected:
	void UpdateStateForNeoHudElementDraw() final;
	void DrawNeoHudElement() final;
	ConVar *GetUpdateFrequencyConVar() const final;

private:
	void DrawPlayerCard(const SpectatorPlayerCard &card,
			const bool bIsLeftSide,
			const int x, const int y,
			const int wide, const int tall,
			const Color accentColor, const float flWideAs43) const;

	vgui::HFont m_hNameFont = 0;
	vgui::HFont m_hInfoFont = 0;
	vgui::HFont m_hClassFont = 0;
	vgui::HFont m_hRKHPFont = 0;
	vgui::HFont m_hGhostFont = 0;
	vgui::HFont m_hSmallWeaponsFont = 0;
	int m_iDeadTex = 0;

	SpectatorPlayerCard m_cards[MAX_PLAYERS_ARRAY_SAFE] = {};
	int m_iCardsSize = 0;
	int m_iTeamPlayersCount[TEAM__TOTAL] = {};
};

