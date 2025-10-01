#ifndef NEO_HUD_SPECTATOR_TAKEOVER_H
#define NEO_SPECTATOR_TAKEOVER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "vgui_controls/Panel.h"
#include "neo_hud_childelement.h"
#include "neo_ui.h"

//-----------------------------------------------------------------------------
// Purpose: Displays a hint when spectating a potential takeover target
//-----------------------------------------------------------------------------
class CNEOHud_SpectatorTakeover : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_SpectatorTakeover, vgui::Panel);

public:
	CNEOHud_SpectatorTakeover(const char *pElementName);
	~CNEOHud_SpectatorTakeover();

	void Init();
	void VidInit();
	void Reset();
	bool ShouldDraw();

	virtual void FireGameEvent( IGameEvent * event );

protected:
	void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;
	virtual void Paint() override;

private:
	NeoUI::Context m_uiCtx;
	wchar_t m_wszHintText[32];

	float m_flDisplayTime;
	bool m_bIsDisplayingHint;
	bool m_bHintShownForCurrentTarget;
	CHandle<C_BasePlayer> m_hLastSpectatedTarget;
};

#endif // NEO_HUD_SPECTATOR_TAKEOVER_H
