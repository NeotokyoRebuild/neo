#pragma once
#include "neo_hud_worldpos_marker.h"

class C_NEOWorldPosMarkerEnt;

class CNEOHud_WorldPosMarker_Generic : public CNEOHud_WorldPosMarker
{
	DECLARE_CLASS_SIMPLE( CNEOHud_WorldPosMarker_Generic, CNEOHud_WorldPosMarker );

public:
	CNEOHud_WorldPosMarker_Generic( const char *pElementName, C_NEOWorldPosMarkerEnt *src, vgui::Panel *parent = nullptr );
	CNEOHud_WorldPosMarker_Generic( const CNEOHud_WorldPosMarker_Generic &other ) = delete;

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar *GetUpdateFrequencyConVar() const override;

	virtual bool ShouldDraw() override;

private:
	C_NEOWorldPosMarkerEnt *m_pSource = nullptr;
	int m_iPosX = 0;
	int m_iPosY = 0;
	float m_flDistance = 0.0f;
	float m_fMarkerScalesStart[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	float m_fMarkerScalesCurrent[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	wchar_t m_wszMarkerTextUnicode[64 + 1] = {};
	Vector m_vecMyPos = vec3_origin;
	vgui::HFont m_hFont = vgui::INVALID_FONT;
	vgui::HTexture m_hSprite[MAX_SCREEN_OVERLAYS] = {};
};
