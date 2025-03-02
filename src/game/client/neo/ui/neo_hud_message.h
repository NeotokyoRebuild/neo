#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "tier0/valve_minmax_off.h"
#include <vector>
#include <string>
#include "tier0/valve_minmax_on.h"

class CNEOHud_Message : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
    DECLARE_CLASS_SIMPLE(CNEOHud_Message, Panel);

public:
    CNEOHud_Message(const char* pElementName, vgui::Panel* parent = NULL);

    void ApplySchemeSettings(vgui::IScheme* pScheme) override;
    virtual void Paint() override;

    void ShowMessage(wchar_t* message);
    void HideMessage();

    void Reset() override;
protected:
    virtual void UpdateStateForNeoHudElementDraw() override;
    virtual void DrawNeoHudElement() override;
    virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
    int m_iCornerTexture;
    int m_iBackgroundTexture;
    vgui::HFont m_hFont;
    vgui::HFont m_hTitleFont;

    wchar_t m_szMessage[256];
    bool m_bShouldDraw;
    float m_fScale;

    std::vector<std::wstring> m_Lines;
    std::vector<int> m_LineHeights;
    int m_BoxWidth;
    int m_BoxHeight;
    int m_Padding;

    CPanelAnimationVarAliasType(int, xpos, "xpos", "80", "proportional_xpos");
    CPanelAnimationVarAliasType(int, ypos, "ypos", "60", "proportional_ypos");
    CPanelAnimationVarAliasType(int, wide, "wide", "300", "proportional_xpos");
    CPanelAnimationVarAliasType(int, tall, "tall", "200", "proportional_ypos");
};
