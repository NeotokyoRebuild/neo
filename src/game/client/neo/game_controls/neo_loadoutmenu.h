#ifndef NEO_LOADOUT_MENU_H
#define NEO_LOADOUT_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include "neo/game_controls/neo_button.h"
#include "neo/game_controls/neo_image_button.h"

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

	GameActionSet_t GetPreferredActionSet() override { return GAME_ACTION_SET_IN_GAME_HUD; }

	virtual void OnMessage(const KeyValues *params, vgui::VPANEL fromPanel);

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();

	virtual void OnMousePressed(vgui::MouseCode code);

	const char *GetResFile(void) const
	{
		return "resource/neo_ui/Neo_LoadoutMenu_Dev.res";
	}

protected:
	virtual void OnClose() override;
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
	vgui::CNeoImageButton *m_pButton1;
	vgui::CNeoImageButton *m_pButton2;
	vgui::CNeoImageButton *m_pButton3;
	vgui::CNeoImageButton *m_pButton4;
	vgui::CNeoImageButton *m_pButton5;
	vgui::CNeoImageButton *m_pButton6;
	vgui::CNeoImageButton *m_pButton7;
	vgui::CNeoImageButton *m_pButton8;
	vgui::CNeoImageButton *m_pButton9;
	vgui::CNeoImageButton *m_pButton10;
	vgui::CNeoImageButton *m_pButton11;
	vgui::CNeoImageButton *m_pButton12;
	vgui::CNeoButton *returnButton;

	// Our viewport interface accessor
	IViewPort *m_pViewPort;

	bool m_bLoadoutMenu;

private:
	int m_iLoadoutNone;
};
#endif // NEO_LOADOUT_MENU_H
