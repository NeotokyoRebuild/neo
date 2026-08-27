#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include "c_neo_point_world_text.h"
#include <vgui_controls/Panel.h>

enum NavErrorType
{
	NAV_OK,
	NAV_CANT_ACCESS_FILE,
	NAV_INVALID_FILE,
	NAV_BAD_FILE_VERSION,
	NAV_FILE_OUT_OF_DATE,
	NAV_CORRUPT_DATA,
	NAV_OUT_OF_MEMORY,
};

class CNEOHud_PlaceName : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel, public CAutoGameSystem
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

	NavErrorType GetNavDataFromFile(CUtlBuffer& outBuffer, bool* pNavDataFromBSP = nullptr);
	void GetPlacesFromNavFile();

	// CAutoGameSystem
	
	virtual void LevelInitPostEntity() override
	{
		GetPlacesFromNavFile();
	}
	
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