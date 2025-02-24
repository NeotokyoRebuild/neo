#ifndef NEO_HUD_WORLDPOS_MARKER_H
#define NEO_HUD_WORLDPOS_MARKER_H
#ifdef _WIN32
#pragma once
#endif

#pragma once
#include "hudelement.h"
#include "neo_hud_childelement.h"
#include "vgui_controls/Panel.h"

abstract_class CNEOHud_WorldPosMarker : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
    DECLARE_CLASS_SIMPLE(CNEOHud_WorldPosMarker, Panel)

public:
    CNEOHud_WorldPosMarker(const char* pElementName, vgui::Panel* parent);
    virtual ~CNEOHud_WorldPosMarker() { }

    void ApplySchemeSettings(vgui::IScheme* pScheme) OVERRIDE;    

protected:
    int m_viewWidth, m_viewHeight;
    float m_viewCentreSize;

    static Color FadeColour(const Color& originalColour, float alphaMultiplier);

    float GetFadeValueTowardsScreenCentre(int x0, int x1, int y0, int y1) const;
    float GetFadeValueTowardsScreenCentre(int x, int y) const;
    float GetFadeValueTowardsScreenCentreInverted(int x, int y, float min) const;
    float GetFadeValueTowardsScreenCentreInAndOut(int x, int y, float min) const;
    float DistanceToCentre(int x, int y) const;
    static void RectToPoint(int x0, int x1, int y0, int y1, int& x, int& y);

private:
    int m_viewCentreX, m_viewCentreY;
    
};
#endif //NEO_HUD_WORLDPOS_MARKER_H
