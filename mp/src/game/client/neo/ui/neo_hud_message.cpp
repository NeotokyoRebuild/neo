#include "cbase.h"
#include "neo_hud_message.h"
#include "iclientmode.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui/IScheme.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_Message, neo_message);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Message, 0.5);

CNEOHud_Message::CNEOHud_Message(const char* pElementName, vgui::Panel* parent)
    : CHudElement(pElementName), Panel(parent, pElementName)
{
    SetAutoDelete(true);

    if (parent) {
        SetParent(parent);
    }
    else {
        SetParent(g_pClientMode->GetViewport());
    }

    m_iCornerTexture = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile(m_iCornerTexture, "vgui/hud/cornercross", true, false);

    m_iBackgroundTexture = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile(m_iBackgroundTexture, "vgui/hud/backgroundcross", true, false);

    m_hFont = 0;
    m_hTitleFont = 0;
    m_szMessage[0] = L'\0';
    m_bShouldDraw = true;
}

void CNEOHud_Message::Reset()
{
    HideMessage();
}


void CNEOHud_Message::ApplySchemeSettings(vgui::IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_hFont = pScheme->GetFont("NHudMessage", IsProportional());
    m_hTitleFont = pScheme->GetFont("NHudMessageTitle", IsProportional());

    int screenWidth, screenHeight;
    surface()->GetScreenSize(screenWidth, screenHeight);

    float baseWide = 720.f;
    float baseTall = 576.f;

    float xRatio = screenWidth / 640.f;
    float yRatio = screenHeight / 480.f;
    float finalRatio = xRatio / yRatio;

    SetBounds(xpos * finalRatio, ypos * finalRatio, wide * finalRatio, tall * finalRatio);

    // SCALE... EVERYTHING!!
    m_fScale = ((wide * finalRatio) / baseWide < (tall * finalRatio) / baseTall) ?
               (wide * finalRatio) / baseWide : (tall * finalRatio) / baseTall;
}

void CNEOHud_Message::DrawNeoHudElement()
{
    if (!m_bShouldDraw || !m_szMessage[0])
    {
        return;
    }

    // Background box
    surface()->DrawSetColor(Color(0, 0, 0, 150));
    surface()->DrawFilledRect(0, 0, m_BoxWidth, m_BoxHeight);

    // Background crosses
    surface()->DrawSetColor(Color(255, 255, 255, 50));
    surface()->DrawSetTexture(m_iBackgroundTexture);
    const int tileSize = (int)(25 * m_fScale);
    for (int x = 0; x <= m_BoxWidth; x += tileSize)
    {
        for (int y = 0; y <= m_BoxHeight; y += tileSize)
        {
            surface()->DrawTexturedRect(x, y, x + tileSize, y + tileSize);
        }
    }

    // Separate lines of text
    int yOffset = m_Padding;
    surface()->DrawSetTextFont(m_hFont);
    surface()->DrawSetTextColor(255, 255, 255, 255);
    for (size_t i = 0; i < m_Lines.size(); i++)
    {
        surface()->DrawSetTextPos(m_Padding, yOffset);
        const std::wstring& line = m_Lines[i];
        surface()->DrawPrintText(line.c_str(), line.length());
        yOffset += m_LineHeights[i] + (int)(2 * m_fScale);
    }

    // Corner sprites
    int crossSize = (int)(12 * m_fScale);
    int crossInset = (int)(6 * m_fScale);
    surface()->DrawSetColor(255, 255, 255, 255);
    surface()->DrawSetTexture(m_iCornerTexture);

    // TopL
    surface()->DrawTexturedRect(crossInset, crossInset, crossSize + crossInset, crossSize + crossInset);
    // TopR
    surface()->DrawTexturedRect(m_BoxWidth - crossSize - crossInset, crossInset, m_BoxWidth - crossInset, crossSize + crossInset);
    // BottomL
    surface()->DrawTexturedRect(crossInset, m_BoxHeight - crossSize - crossInset, crossSize + crossInset, m_BoxHeight - crossInset);
    // BottomR
    surface()->DrawTexturedRect(m_BoxWidth - crossSize - crossInset, m_BoxHeight - crossSize - crossInset, m_BoxWidth - crossInset, m_BoxHeight - crossInset);

    // Epic style points title text
    surface()->DrawSetTextFont(m_hTitleFont);
    surface()->DrawSetTextColor(255, 255, 255, 255);
    surface()->DrawSetTextPos(crossInset + (int)(20 * m_fScale), crossInset);
    surface()->DrawPrintText(L"SYSTEM_MSG", 10);
}

void CNEOHud_Message::Paint()
{
    BaseClass::Paint();
    PaintNeoElement();
}

void CNEOHud_Message::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_Message::ShowMessage(wchar_t* message)
{
    wcsncpy(m_szMessage, message, sizeof(m_szMessage) / sizeof(wchar_t));
    m_bShouldDraw = true;

    m_Lines.clear();
    m_LineHeights.clear();

    m_Padding = (int)(40 * m_fScale);
    int maxWidth = 0;
    int totalHeight = 0;

    wchar_t* messageCopy = wcsdup(message);
    wchar_t* context = nullptr;
    wchar_t* token = wcstok(messageCopy, L"\n", &context); // Any \n in the localised text will declare a new line

    while (token)
    {
        std::wstring line(token);
        m_Lines.push_back(line);

        int textWidth = 0, textHeight = 0;
        surface()->GetTextSize(m_hFont, line.c_str(), textWidth, textHeight);
        m_LineHeights.push_back(textHeight);

        if (textWidth > maxWidth)
            maxWidth = textWidth;

        totalHeight += textHeight + 2;
        token = wcstok(nullptr, L"\n", &context);
    }
    free(messageCopy);

    m_BoxWidth = maxWidth + (m_Padding * 2);
    m_BoxHeight = totalHeight + (m_Padding * 2);
}

void CNEOHud_Message::HideMessage()
{
    m_szMessage[0] = L'\0';
    m_bShouldDraw = false;
}
