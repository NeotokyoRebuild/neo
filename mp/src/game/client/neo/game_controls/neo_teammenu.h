#ifndef NEO_TEAM_MENU_H
#define NEO_TEAM_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <igameevents.h>
#include "GameEventListener.h"

#include <game/client/iviewport.h>

namespace vgui
{
	class Button;
    class ComboBox;
    class Label;
	class Panel;
    class TextEntry;
    //class ImagePanel;
}
class IconPanel;

class MouseCode;

// NOTE: this class name must match its res file description.
class CNeoTeamMenu : public vgui::Frame,
    public IViewPortPanel
{
    DECLARE_CLASS_SIMPLE( CNeoTeamMenu, vgui::Frame );

    MESSAGE_FUNC_PARAMS(OnButtonPressed, "PressButton", data);

public:
    CNeoTeamMenu(IViewPort *pViewPort);
    virtual ~CNeoTeamMenu();

    virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void SetData(KeyValues *data) { }
	virtual void Reset() { }
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

    // both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

    virtual Color GetBlackBarColor( void ) { return Color(0, 0, 0, 196); }

    virtual const char *GetResFile(void)
    {
        return "resource/neo_ui/Neo_TeamMenu.res";
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

    // --------------------------------------------------------
    // Menu pieces. These are defined in the GetResFile() file.
    // --------------------------------------------------------

    // Image textures
	//vgui::ImagePanel *m_pJinrai_TeamImage;
	//vgui::ImagePanel *m_pNSF_TeamImage;

    vgui::ImagePanel* m_pJinrai_TeamImage;
    vgui::ImagePanel* m_pNSF_TeamImage;
    vgui::ImagePanel* m_pBgDarkGrey;
#if(1)
	//vgui::ImagePanel *m_pBackgroundImage;

    // Team menu label
    vgui::Label *m_pTeamMenuLabel;

    // Jinrai playercount & score labels
    vgui::Label *m_pJinrai_PlayercountLabel;
    vgui::Label *m_pJinrai_ScoreLabel;
    // NSF playercount & score labels
    vgui::Label *m_pNSF_PlayercountLabel;
    vgui::Label *m_pNSF_ScoreLabel;

    // Divider
    vgui::Divider *m_pDivider;
#endif

    // Buttons
    vgui::Button *m_pJinrai_Button;
    vgui::Button *m_pNSF_Button;
    vgui::Button *m_pSpectator_Button;
    vgui::Button *m_pAutoAssign_Button;
    vgui::Button *m_pCancel_Button;

    // Our viewport interface accessor
    IViewPort *m_pViewPort;

    // Which team to highlight for auto selection
    int m_iDefaultTeam;

    bool m_bTeamMenu;

protected:
	void CommandCompletion();

private:
    inline vgui::Button *GetPressedButton();
};

extern CNeoTeamMenu *g_pNeoTeamMenu;

#endif // NEO_TEAM_MENU_H