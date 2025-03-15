#include <vgui/ISurface.h>
#include "neo/game_controls/neo_button.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CNeoButton, NeoButton );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNeoButton::GetButtonTextures()
{
	m_iTextureArmed = vgui::surface()->DrawGetTextureId("vgui/buttons/button_armed");
	if (m_iTextureArmed == -1) // we didn't find it, so create a new one
	{
		m_iTextureArmed = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iTextureArmed, "vgui/buttons/button_armed", true, false);
	}
	m_iTextureDisabled = vgui::surface()->DrawGetTextureId("vgui/buttons/button_disabled");
	if (m_iTextureDisabled == -1) // we didn't find it, so create a new one
	{
		m_iTextureDisabled = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iTextureDisabled, "vgui/buttons/button_disabled", true, false);
	}
	m_iTextureMouseOver = vgui::surface()->DrawGetTextureId("vgui/buttons/button_mouseover");
	if (m_iTextureMouseOver == -1) // we didn't find it, so create a new one
	{
		m_iTextureMouseOver = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iTextureMouseOver, "vgui/buttons/button_mouseover", true, false);
	}
	//m_iTextureNormal = vgui::surface()->DrawGetTextureId("vgui/buttons/button_normal");
	//if (m_iTextureNormal == -1) // we didn't find it, so create a new one
	//{
	//	m_iTextureNormal = vgui::surface()->CreateNewTextureID();
	//	vgui::surface()->DrawSetTextureFile(m_iTextureNormal, "vgui/buttons/button_normal", true, false);
	//}
	m_iTexturePressed = vgui::surface()->DrawGetTextureId("vgui/buttons/button_pressed");
	if (m_iTexturePressed == -1) // we didn't find it, so create a new one
	{
		m_iTexturePressed = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_iTexturePressed, "vgui/buttons/button_pressed", true, false);
	}
}

void CNeoButton::PaintBackground()
{
}

//-----------------------------------------------------------------------------
// Purpose:	Paint NeoButton on screen
//-----------------------------------------------------------------------------
void CNeoButton::Paint(void)
{
	if ( !ShouldPaint() )
		return; 

	int wide, tall;
	GetSize(wide, tall);

	int textureID = m_iTextureArmed;
	if (IsArmed())
	{
		if (IsDepressed())
			textureID = m_iTexturePressed;
		else
			textureID = m_iTextureMouseOver;
	}
	else if (!IsEnabled())
	{
		textureID = m_iTextureDisabled;
	}

	surface()->DrawSetColor(255, 255, 255, 255);
	surface()->DrawSetTexture(textureID);
	surface()->DrawTexturedRect(0, 0, wide, tall);

	BaseClass::Paint();
}

void CNeoButton::PaintBorder()
{
}

void CNeoButton::DrawFocusBorder(int tx0, int ty0, int tx1, int ty1)
{
	BaseClass::DrawFocusBorder(tx0, ty0, tx1, ty1 - 2);
}