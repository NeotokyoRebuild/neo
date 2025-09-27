#include <vgui/ISurface.h>
#include "neo/game_controls/neo_image_button.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

namespace vgui
{

DECLARE_BUILD_FACTORY_DEFAULT_TEXT(CNeoImageButton, NeoImageButton);

//-----------------------------------------------------------------------------
// Purpose:	override default button borders
//-----------------------------------------------------------------------------
void CNeoImageButton::ApplySchemeSettings(IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	_defaultBorder = pScheme->GetBorder("NeoImageButtonBorder");
	_depressedBorder = pScheme->GetBorder("NeoImageButtonDepressedBorder");
	_keyFocusBorder = pScheme->GetBorder("NeoImageButtonKeyFocusBorder");
}

//-----------------------------------------------------------------------------
// Purpose:	Paint Image texture and mouse hover "border"
//-----------------------------------------------------------------------------
void CNeoImageButton::Paint()
{
	if (!ShouldPaint())
	{
		return;
	}

	if (m_iTextureID != -1)
	{
		surface()->DrawSetColor(255, 255, 255, 255);
		surface()->DrawSetTexture(m_iTextureID);
		surface()->DrawTexturedRect(0, 0, wide, tall);
	}

	BaseClass::Paint();

	if (IsArmed() && !IsDepressed())
	{
		surface()->DrawOutlinedRect(3, 3, wide -3, tall -3);
	}
}

void CNeoImageButton::SetButtonTextureID(int textureID)
{
	m_iTextureID = textureID;
}

void CNeoImageButton::SetButtonTexture(const char* texture)
{
	m_iTextureID = surface()->DrawGetTextureId(texture);
	if (m_iTextureID == -1)
	{
		m_iTextureID = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_iTextureID, texture, true, false);
	}
}

} // namespace vgui