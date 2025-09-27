#ifndef NEO_SPECTATOR_TAKEOVER_H
#define NEO_SPECTATOR_TAKEOVER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "vgui_controls/Panel.h"
#include "neo_ui.h"

//-----------------------------------------------------------------------------
// Purpose: Displays a hint when spectating a potential takeover target
//-----------------------------------------------------------------------------
class CNEOHudSpectatorTakeover : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHudSpectatorTakeover, vgui::Panel);

public:
	CNEOHudSpectatorTakeover(const char *pElementName);
	~CNEOHudSpectatorTakeover();

	void Init();
	void VidInit();
	void Reset();
	void OnThink();
	bool ShouldDraw();
	void Paint();

	virtual void FireGameEvent( IGameEvent * event );

protected:
	void ApplySchemeSettings(vgui::IScheme *pScheme) override;

private:
	NeoUI::Context m_uiCtx;
	wchar_t m_wszHintText[32];

	float m_flDisplayTime;
	bool m_bIsDisplayingHint;
	bool m_bHintShownForCurrentTarget;
	CHandle<C_BasePlayer> m_hLastSpectatedTarget;
};

#endif // NEO_SPECTATOR_TAKEOVER_H
