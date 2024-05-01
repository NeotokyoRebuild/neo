#ifndef NEO_CLASS_MENU_H
#define NEO_CLASS_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <igameevents.h>
#include "GameEventListener.h"

#include <game/client/iviewport.h>

namespace vgui
{
	class TextEntry;
	class Button;
	class Panel;
	class ImagePanel;
	class ComboBox;
    class Button;
}

class MouseCode;

// NOTE: this class name must match its res file description.
class CNeoClassMenu : public vgui::Frame,
    public IViewPortPanel
{
    DECLARE_CLASS_SIMPLE( CNeoClassMenu, vgui::Frame );

public:
    CNeoClassMenu(IViewPort *pViewPort);
    virtual ~CNeoClassMenu();

    virtual const char *GetName( void ) { return PANEL_CLASS; }
	virtual void SetData(KeyValues *data) { }
	virtual void Reset() { }
	virtual void Update() { /* Do things! */ }
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

    virtual void OnMessage(const KeyValues *params, vgui::VPANEL fromPanel);

    // both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();

    virtual void OnMousePressed(vgui::MouseCode code);

    virtual Color GetBlackBarColor( void ) { return Color(0, 0, 0, 196); }

    virtual const char *GetResFile(void)
    {
        return "resource/neo_ui/Neo_ClassMenu.res";
    }

protected:
    void OnCommand(const char *command);
    void ChangeMenu(const char *menuName);

    void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);
	void FindButtons();
	void UpdateSkinImages(int classNumber);
	void UpdateTimer() { }

    // vgui overrides
	virtual void PerformLayout() { }
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

    // --------------------------------------------------------
    // Menu pieces. These are defined in the GetResFile() file.
    // --------------------------------------------------------

    // Image panels
    vgui::ImagePanel *m_pSkinPanel1;
    vgui::ImagePanel *m_pSkinPanel2;
    vgui::ImagePanel *m_pSkinPanel3;

    // Buttons
    vgui::Button *m_pRecon_Button;
    vgui::Button *m_pAssault_Button;
    vgui::Button *m_pSupport_Button;
	vgui::Button *m_pSkin1_Button;
	vgui::Button *m_pSkin2_Button;
	vgui::Button *m_pSkin3_Button;
    vgui::Button *m_pBack_Button;

    // Which class to highlight for auto selection
    int m_iDefaultClass;
    bool m_bClassMenu;

protected:
	void CommandCompletion();
};

extern CNeoClassMenu *g_pNeoClassMenu;

#endif // NEO_CLASS_MENU_H