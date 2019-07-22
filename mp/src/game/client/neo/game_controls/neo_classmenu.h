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

#define BLACK_BAR_COLOR	Color(0, 0, 0, 196)

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

    MESSAGE_FUNC_PARAMS(OnButtonPressed, "PressButton", data);

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

    virtual Color GetBlackBarColor( void ) { return BLACK_BAR_COLOR; }

    virtual const char *GetResFile(void)
    {
        return "resource/ui/ClassMenu.res";
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

	vgui::IImage *m_pImage_Jinrai_Recon_Skin1;
	vgui::IImage *m_pImage_Jinrai_Recon_Skin2;
	vgui::IImage *m_pImage_Jinrai_Recon_Skin3;
	vgui::IImage *m_pImage_Jinrai_Assault_Skin1;
	vgui::IImage *m_pImage_Jinrai_Assault_Skin2;
	vgui::IImage *m_pImage_Jinrai_Assault_Skin3;
	vgui::IImage *m_pImage_Jinrai_Support_Skin1;
	vgui::IImage *m_pImage_Jinrai_Support_Skin2;
	vgui::IImage *m_pImage_Jinrai_Support_Skin3;
	vgui::IImage *m_pImage_NSF_Recon_Skin1;
	vgui::IImage *m_pImage_NSF_Recon_Skin2;
	vgui::IImage *m_pImage_NSF_Recon_Skin3;
	vgui::IImage *m_pImage_NSF_Assault_Skin1;
	vgui::IImage *m_pImage_NSF_Assault_Skin2;
	vgui::IImage *m_pImage_NSF_Assault_Skin3;
	vgui::IImage *m_pImage_NSF_Support_Skin1;
	vgui::IImage *m_pImage_NSF_Support_Skin2;
	vgui::IImage *m_pImage_NSF_Support_Skin3;

    // Image panels
    vgui::ImagePanel *m_pSkinPanel1;
    vgui::ImagePanel *m_pSkinPanel2;
    vgui::ImagePanel *m_pSkinPanel3;
	vgui::ImagePanel *m_pChooseClassIcon;
	vgui::ImagePanel *m_pChooseChooseSkinIcon;

    // Class menu label
    vgui::Label *m_pClassMenuLabel;

    // Divider
    vgui::Divider *m_pDivider;

    // Buttons
    vgui::Button *m_pRecon_Button;
    vgui::Button *m_pAssault_Button;
    vgui::Button *m_pSupport_Button;
    vgui::Button *m_pBack_Button;

    // Our viewport interface accessor
    IViewPort *m_pViewPort;

    // Which class to highlight for auto selection
    int m_iDefaultClass;

    bool m_bClassMenu;

protected:
	void CommandCompletion();

private:
    inline vgui::Button *GetPressedButton();
};

extern CNeoClassMenu *g_pNeoClassMenu;

#endif // NEO_CLASS_MENU_H