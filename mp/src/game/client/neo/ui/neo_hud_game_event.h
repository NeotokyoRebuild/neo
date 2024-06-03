#ifndef NEO_HUD_GAME_EVENT_H
#define NEO_HUD_GAME_EVENT_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "vgui_controls/Panel.h"

#define NEO_MAX_HUD_GAME_EVENT_MSG_SIZE 32 + 1

class CNEOHud_GameEvent : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GameEvent, vgui::Panel);
public:
	CNEOHud_GameEvent(const char *pElementName, vgui::Panel *parent = nullptr);

	void SetMessage(const char *message, size_t size);
	void SetMessage(const wchar_t *message, size_t size);
	void MsgFunc_RoundResult(bf_read& msg);

	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	vgui::HFont m_hFont;
	vgui::HTexture jinraiWin, nsfWin, tie;

	int m_iResX, m_iResY;

	char teamWon[64];
	int timeWon;
	char message[64 + MAX_PLACE_NAME_LENGTH];

	wchar_t messageWord[64 + MAX_PLACE_NAME_LENGTH];
	int messageWidth;
	int messageHeight;
	int messageLength;

	wchar_t m_pszMessage[NEO_MAX_HUD_GAME_EVENT_MSG_SIZE];

	CPanelAnimationVarAliasType(int, image_y_offset, "image_y_offset", "60", "proportional_ypos");
	CPanelAnimationVarAliasType(int, text_y_offset, "text_y_offset", "420", "proportional_ypos");
};


#endif // NEO_HUD_GAME_EVENT_H
