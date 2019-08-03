#ifndef NEO_HUD_ELEMENTS_H
#define NEO_HUD_ELEMENTS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "GameEventListener.h"

class CNEOHud_Compass;

class CNeoHudElements : public vgui::EditablePanel,
	public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CNeoHudElements, vgui::EditablePanel);

public:
	CNeoHudElements(IViewPort *pViewPort);
	~CNeoHudElements();

	virtual const char *GetName(void) { return PANEL_NEO_HUD; }
	
	virtual void SetData(KeyValues *data) { }
	virtual void Reset();
	virtual void Update();
	virtual void ShowPanel(bool bShow);

	virtual bool NeedsUpdate(void);
	virtual bool HasInputElements() { return false; }

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event);

	virtual void UpdatePlayerIFF(int playerIndex, KeyValues *kv);

	CNEOHud_Compass *GetCompass();

protected:
	virtual void OnThink();
	virtual int GetAdditionalHeight() { return 0; }

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PostApplySchemeSettings(vgui::IScheme *pScheme);

	int FindIFFItemIDForPlayerIndex(int playerIndex);

	CPanelAnimationVar(int, m_iIFFIconWidth, "iff_icon_width", "15");

	float m_fNextUpdateTime;

	void InitHud();

private:
	void InitCompass();

	int m_iPlayerIndexSymbol;

	IViewPort *m_pViewPort;

	CNEOHud_Compass *m_pCompass;

	void FillIFFs();
};

#endif // NEO_HUD_ELEMENTS_H