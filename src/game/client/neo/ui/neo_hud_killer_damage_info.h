#pragma once

#include "neo_hud_childelement.h"
#include "c_neo_killer_damage_infos.h"
#include "neo_ui.h"
#include "neo_player_shared.h"

#include <hudelement.h>
#include <vgui_controls/Panel.h>

enum KDMGINFO_TOGGLETYPE
{
	KDMGINFO_TOGGLETYPE_ROUND = 0, // m_bShowInfo always set to true on death/end, per round toggle
	KDMGINFO_TOGGLETYPE_MATCH, // m_bShowInfo not changed on death/end, per match toggle
	KDMGINFO_TOGGLETYPE_NEVER, // m_bShowInfo always set to false on death/end, per round toggle

	KDMGINFO_TOGGLETYPE__TOTAL,
};

class CNEOHud_KillerDamageInfo : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_KillerDamageInfo, CNEOHud_ChildElement)

public:
	CNEOHud_KillerDamageInfo(const char *pszName, vgui::Panel *parent = nullptr);
	~CNEOHud_KillerDamageInfo();

	void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	void Paint() override;
	void resetHUDState() override;

	bool m_bShowInfo = true;
	bool m_bPlayerShownHud = false;

	int m_iTotalPages = 0;
	int m_iCurPage = 0;

protected:
	void UpdateStateForNeoHudElementDraw() override;
	void DrawNeoHudElement() override;
	ConVar *GetUpdateFrequencyConVar() const override;
	void ResetDisplayInfos();

	vgui::HFont m_hFont;
	NeoUI::Context m_uiCtx;

	wchar_t m_wszKillerTitle[256] = {};
	wchar_t m_wszBindBtnMsg[128] = {};

	wchar_t m_wszDmgerNamesList[MAX_PLAYERS][MAX_PLAYER_NAME_LENGTH + 1] = {};
	AttackersTotals m_attackersTotals[MAX_PLAYERS] = {};
	int m_iClassIdxsList[MAX_PLAYERS] = {};
	int m_iTeamIdxsList[MAX_PLAYERS] = {};
	int m_iEntIdxsList[MAX_PLAYERS] = {};
	int m_iAttackersTotalsSize = 0;
};
