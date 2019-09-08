#ifndef NEO_LOADOUT_MENU_H
#define NEO_LOADOUT_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>

#define CNeoLoadoutMenu CNeoLoadoutMenu_Dev

class CNeoLoadoutMenu : public vgui::Frame, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE(CNeoLoadoutMenu, vgui::Frame);

	MESSAGE_FUNC_PARAMS(OnButtonPressed, "PressButton", data);

public:
	CNeoLoadoutMenu(IViewPort *pViewPort);
	virtual ~CNeoLoadoutMenu();

	void CommandCompletion();

	virtual const char *GetName(void) { return PANEL_NEO_LOADOUT; }
	virtual void SetData(KeyValues *data) { }
	virtual void Reset() { }
	virtual void Update() { /* Do things! */ }
	virtual bool NeedsUpdate(void) { return false; }
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);

	virtual void OnMessage(const KeyValues *params, vgui::VPANEL fromPanel);

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();

	virtual void OnMousePressed(vgui::MouseCode code);

	virtual const char *GetResFile(void)
	{
		return "resource/ui/LoadoutMenu_Dev.res";
	}

protected:
	void OnCommand(const char *command);

	void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);
	void UpdateTimer() { }

	// vgui overrides
	virtual void PerformLayout() { }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	bool m_bLoadoutMenu;

	// --------------------------------------------------------
	// Menu pieces. These are defined in the GetResFile() file.
	// --------------------------------------------------------

	vgui::ImagePanel *m_pWeapon_ImagePanel;

	vgui::Label *m_pTitleLabel;

	vgui::Button *m_pScout_Button;
	vgui::Button *m_pMisc2;
	vgui::Button *m_pDone_Button;
	vgui::Button *m_pButton1;
	vgui::Button *m_pButton2;
	vgui::Button *m_pButton3;
	vgui::Button *m_pButton4;
	vgui::Button *m_pButton5;
	vgui::Button *m_pButton6;
	vgui::Button *m_pButton7;
	vgui::Button *m_pButton8;
	vgui::Button *m_pButton9;
	vgui::Button *m_pButton10;
	vgui::Button *m_pButton11;
	vgui::Button *m_pButton12;
	vgui::Button *m_pButton13;
	vgui::Button *m_pButton14;

	// Our viewport interface accessor
	IViewPort *m_pViewPort;
};

#endif // NEO_LOADOUT_MENU_H