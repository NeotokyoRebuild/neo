#ifndef NEO_IMAGE_BUTTON_H
#define NEO_IMAGE_BUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Button.h>

namespace vgui
{

class CNeoImageButton : public Button
{
	DECLARE_CLASS_SIMPLE(CNeoImageButton, Button);

public:
	CNeoImageButton(Panel* parent, const char* panelName, const char* text, Panel* pActionSignalTarget = NULL, const char* pCmd = NULL) : Button(parent, panelName, text, pActionSignalTarget, pCmd) {};
	CNeoImageButton(Panel* parent, const char* panelName, const wchar_t* text, Panel* pActionSignalTarget = NULL, const char* pCmd = NULL) : Button(parent, panelName, text, pActionSignalTarget, pCmd) {};

	void SetButtonTextureID(int textureID);
	void SetButtonTexture(const char* texture);

protected:
	void ApplySchemeSettings(IScheme* pScheme) override;
	void Paint(void) override;

private:
	CPanelAnimationVarAliasType(int, wide, "wide", "256", "int");
	CPanelAnimationVarAliasType(int, tall, "tall", "256", "int");

	int	m_iTextureID = -1;
};

} // namespace vgui

#endif // NEO_IMAGE_BUTTON_H
