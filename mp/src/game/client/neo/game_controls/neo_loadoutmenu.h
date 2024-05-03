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
		return "resource/neo_ui/Neo_LoadoutMenu_Dev.res";
	}

protected:
	void OnCommand(const char *command);
	void ChangeMenu(const char* menuName);
	void OnKeyCodeReleased(vgui::KeyCode code);

	void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);
	void FindButtons();
	void UpdateTimer() { }

	// vgui overrides
	virtual void PerformLayout() { }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// --------------------------------------------------------
	// Menu pieces. These are defined in the GetResFile() file.
	// --------------------------------------------------------
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
	vgui::Button *returnButton;

	// Our viewport interface accessor
	IViewPort *m_pViewPort;

	bool m_bLoadoutMenu;
};
#endif // NEO_LOADOUT_MENU_H
