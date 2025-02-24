#include "cbase.h"
#include "neo_hud_worldpos_marker.h"
#include "vgui/ISurface.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;


ConVar neo_cl_hud_centre_size("neo_cl_hud_centre_size", "25", FCVAR_USERINFO,
                              "HUD centre size in percentage to fade markers.", true, 1, false, 0);

CNEOHud_WorldPosMarker::CNEOHud_WorldPosMarker(const char* pElementName, Panel* parent)
    : CHudElement(pElementName),
      Panel(parent, pElementName),
      m_viewWidth(0),
      m_viewHeight(0),
      m_viewCentreSize(0),
      m_viewCentreX(0),
      m_viewCentreY(0)
{
}

void CNEOHud_WorldPosMarker::ApplySchemeSettings(vgui::IScheme* pScheme)
{
    surface()->GetScreenSize(m_viewWidth, m_viewHeight);
    SetBounds(0, 0, m_viewWidth, m_viewHeight);

    m_viewCentreX = m_viewWidth / 2;
    m_viewCentreY = m_viewHeight / 2;

    auto widerAxis = max(m_viewWidth, m_viewHeight);
    m_viewCentreSize = widerAxis * (static_cast<float>(neo_cl_hud_centre_size.GetInt()) / 100);

    BaseClass::ApplySchemeSettings(pScheme);
}

void CNEOHud_WorldPosMarker::RectToPoint(int x0, int x1, int y0, int y1, int& x, int& y)
{
    x = (x0 - x1) / 2;
    y = (y0 - y1) / 2;
}

float CNEOHud_WorldPosMarker::DistanceToCentre(int x, int y) const
{
    
    return static_cast<float>(sqrt(pow(m_viewCentreX - x, 2) + pow(y - m_viewCentreY, 2)));
}

float CNEOHud_WorldPosMarker::GetFadeValueTowardsScreenCentre(int x, int y) const
{
    float innerArea = m_viewCentreSize / 2.f;
    auto dist = DistanceToCentre(x, y);
    if(dist <= innerArea)
    {
        return 1;
    }

    if(dist <= m_viewCentreSize)
    {
        return (innerArea - (dist - innerArea)) / innerArea;
    }

    return 0;
}

float CNEOHud_WorldPosMarker::GetFadeValueTowardsScreenCentreInverted(int x, int y, float min) const
{
    float innerArea = m_viewCentreSize / 2.f;
    auto dist = DistanceToCentre(x, y);
    if(dist <= innerArea)
    {
        return min;
    }

    if(dist <= m_viewCentreSize)
    {
        return max(min, (dist - innerArea) / innerArea);
    }

    return 1;
}

float CNEOHud_WorldPosMarker::GetFadeValueTowardsScreenCentreInAndOut(int x, int y, float min) const
{
    float innerArea = m_viewCentreSize / 2.f;
    auto dist = DistanceToCentre(x, y);
    if(dist <= innerArea)
    {
        return max(min, dist / innerArea);
    }

    if(dist <= m_viewCentreSize)
    {
        return (innerArea - (dist - innerArea)) / innerArea;
    }

    return 0;
}

float CNEOHud_WorldPosMarker::GetFadeValueTowardsScreenCentre(int x0, int x1, int y0, int y1) const
{
    int x;
    int y;
    RectToPoint(x0, x1, y0, y1, x, y);
    return GetFadeValueTowardsScreenCentre(x, y);
}

Color CNEOHud_WorldPosMarker::FadeColour(const Color& originalColour, float alphaMultiplier)
{
    return {originalColour.r(), originalColour.g(), originalColour.b(), static_cast<int>(originalColour.a() * alphaMultiplier) };
}
