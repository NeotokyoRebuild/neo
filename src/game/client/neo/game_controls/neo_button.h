#ifndef NEO_BUTTON_H
#define NEO_BUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Button.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNeoButton : public Button
{
	DECLARE_CLASS_SIMPLE( CNeoButton, Button );

public:
	// You can optionally pass in the panel to send the click message to and the name of the command to send to that panel.
	CNeoButton(Panel* parent, const char* panelName, const char* text, Panel* pActionSignalTarget = NULL, const char* pCmd = NULL)
		: Button(parent, panelName, text, pActionSignalTarget, pCmd)
	{
		GetButtonTextures();
	};
	CNeoButton(Panel* parent, const char* panelName, const wchar_t* text, Panel* pActionSignalTarget = NULL, const char* pCmd = NULL)
		: Button(parent, panelName, text, pActionSignalTarget, pCmd)
	{
		GetButtonTextures();
	};

protected:
	//virtual void DrawFocusBorder(int tx0, int ty0, int tx1, int ty1);

	// Paint Button on screen
	virtual void Paint(void) override;
	virtual void PaintBackground() override;
	virtual void PaintBorder();
	virtual void DrawFocusBorder(int tx0, int ty0, int tx1, int ty1);

protected:
	void GetButtonTextures();

	int					m_iTextureArmed;
	int					m_iTextureDisabled;
	int					m_iTextureMouseOver;
	//int					m_iTextureNormal;
	int					m_iTexturePressed;
};

} // namespace vgui

#endif // NEO_BUTTON_H
