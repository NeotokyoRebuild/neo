#ifndef NEO_HUD_SPECTATOR_TAKEOVER_H
#define NEO_SPECTATOR_TAKEOVER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "vgui_controls/EditablePanel.h"
#include "neo_hud_childelement.h"

//-----------------------------------------------------------------------------
// Purpose: Displays a hint when spectating a potential takeover target
//-----------------------------------------------------------------------------
class CNEOHud_SpectatorTakeover : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_SpectatorTakeover, vgui::EditablePanel);

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
	wchar_t m_wszHintText[32];

	float m_flDisplayTime;
	bool m_bHintShownForCurrentTarget;
	CHandle<C_BasePlayer> m_hLastSpectatedTarget;

	CPanelAnimationVar(vgui::HFont, m_hHintFont, "font", "NeoUINormal");
	CPanelAnimationVarAliasType(int, m_iPaddingX, "padding_x", "4", "proportional_xpos");
	CPanelAnimationVarAliasType(int, m_iPaddingY, "padding_y", "6", "proportional_ypos");
	CPanelAnimationVar(float, m_flBoxYFactor, "box_y_factor", "0.75");
	CPanelAnimationVar(Color, m_BoxColor, "box_color", "20 20 20 220");
	CPanelAnimationVar(Color, m_TextColor, "text_color", "255 255 255 255");
};

#endif // NEO_HUD_SPECTATOR_TAKEOVER_H