#ifndef NEO_HUD_GAME_EVENT_H
#define NEO_HUD_GAME_EVENT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "vgui_controls/Panel.h"

#define NEO_MAX_HUD_GAME_EVENT_MSG_SIZE 32 + 1

class CNEOHud_GameEvent : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GameEvent, Panel);

public:
	CNEOHud_GameEvent(const char *pElementName, vgui::Panel *parent = NULL);

	void SetMessage(const char *message);
	void SetMessage(const wchar_t *message, size_t size);

	virtual void Paint();

private:
	vgui::HFont m_hFont;

	int m_iResX, m_iResY;

	wchar_t m_pszMessage[NEO_MAX_HUD_GAME_EVENT_MSG_SIZE * sizeof(wchar_t)];
};


#endif // NEO_HUD_GAME_EVENT_H