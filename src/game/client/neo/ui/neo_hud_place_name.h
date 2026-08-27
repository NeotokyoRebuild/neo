#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class PointWorldText
{
public:
	PointWorldText();
	PointWorldText(const char* pszText, Vector pos);
	~PointWorldText();

	int DrawModel();

	void SetText(const char* pszText);
	void SetFont(int nFont);

	Vector GetAbsOrigin() { return m_vecAbsOrigin; }
	QAngle GetAbsAngles() { return m_vecAbsAngles; }

private:
	void CalcTextTotalSize(float &outWidth, float &outHeight);
	void UpdateTextWorldSize();

	float GetTextWorldWidth() const;
	float GetTextWorldHeight() const;
	float GetTextSpacingX() const;
	float GetTextSpacingY() const;

	Vector m_vecAbsOrigin = {0, 0, 0};
	QAngle m_vecAbsAngles = {0, 0, 0};

	char m_szText[ MAX_PLACE_NAME_LENGTH ];
	float m_flTextSize = 100.f;
	float m_flTextSpacingX = 0.f;
	float m_flTextSpacingY = 0.f;
	color32 m_colTextColor = {255, 255, 255, 255};
	int m_nOrientation = 2;
	int m_nTextLength = 0;

	float m_flTextWorldWidth = 0.f;
	float m_flTextWorldHeight = 0.f;
};

class CNEOHud_PlaceName : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_PlaceName, Panel);

public:
	CNEOHud_PlaceName(const char *pElementName, vgui::Panel *parent = NULL);
	~CNEOHud_PlaceName();
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	virtual void Paint() override;
	void DrawPlaceNames();
	CMaterialReference GetFont() const { return m_Font; };

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	CNEOHud_PlaceName(const CNEOHud_PlaceName&other);
	
    wchar_t m_szPlaceName[MAX_PLACE_NAME_LENGTH];
	int textXOffset = 0;
	int wide = 0;

	CUtlVector<PointWorldText> places;
	CMaterialReference m_Font;

    CPanelAnimationVarAliasType(int, textXpos, "textXpos", "80", "proportional_xpos");
    CPanelAnimationVarAliasType(int, textYpos, "textYpos", "80", "proportional_ypos");
	CPanelAnimationVar(vgui::HFont, textFont, "textFont", "");
	CPanelAnimationVar(Color, textColor, "textColor", "255 255 255 255");
	CPanelAnimationVar(int, textXAlignment, "textXAlignment", "0");
};

CNEOHud_PlaceName* GetPlaceName();